// 알리는 쪽 대기 시간(꼬리 지표) 측정.
// 각 반복: pending++ → Notify()(상태 전환, 이 구간만 측정) → 실행권을 얻었으면 Consume() → 다른 작업.
// Notify 구간은 "실행권을 얻었거나, 표시만 하고 떠나도 된다고 판단할 때까지" 걸린 시간이다.
//  - cas      : Busy|Dirty CAS 루프 (Session::_recvState 방식)
//  - casfast  : cas + 이미 예약돼 있으면 load만 하고 반환 (Group::RequestUpdate 방식)
//  - lockflag : 같은 플래그를 mutex로 보호, 소비는 락 밖
//  - coarse   : 락을 얻는 것이 곧 실행권, 소비 전체를 락 안에서 수행
#include <intrin.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <cstdlib>

static double gItersPerNs = 1.0;
static double gCyclesPerNs = 1.0;

static void Spin(long long iters)
{
    for (volatile long long i = 0; i < iters; ++i) {}
}
static void Work(int ns)
{
    if (ns > 0) Spin(static_cast<long long>(ns * gItersPerNs));
}
static void Calibrate()
{
    using namespace std::chrono;
    const long long n = 200'000'000;
    auto b = steady_clock::now();
    Spin(n);
    auto e = steady_clock::now();
    gItersPerNs = n / double(duration_cast<nanoseconds>(e - b).count());

    b = steady_clock::now();
    const auto c0 = __rdtsc();
    while (steady_clock::now() - b < milliseconds(300)) {}
    const auto c1 = __rdtsc();
    e = steady_clock::now();
    gCyclesPerNs = double(c1 - c0) / duration_cast<nanoseconds>(e - b).count();
}

// 로그 버킷 히스토그램: 2의 거듭제곱 구간을 16등분 (해상도 약 6%)
struct Histogram
{
    static constexpr int Sub = 16;
    static constexpr int Buckets = 64 * Sub;
    std::array<unsigned long long, Buckets> count{};

    static int Index(unsigned long long v)
    {
        if (v < Sub) return int(v);
        unsigned long idx;
        _BitScanReverse64(&idx, v);
        const int shift = int(idx) - 4;
        const int mant = int((v >> shift) & (Sub - 1));
        return (shift + 1) * Sub + mant;
    }
    static unsigned long long Upper(int i)
    {
        if (i < Sub) return i;
        const int shift = i / Sub - 1;
        const int mant = i % Sub;
        return ((unsigned long long)(Sub + mant + 1) << shift) - 1;
    }
    void Add(unsigned long long v) { ++count[Index(v)]; }
    void Merge(const Histogram& o) { for (int i = 0; i < Buckets; ++i) count[i] += o.count[i]; }
    double PercentileNs(double p) const
    {
        unsigned long long total = 0;
        for (auto c : count) total += c;
        const auto target = (unsigned long long)std::ceil(total * p);
        unsigned long long acc = 0;
        for (int i = 0; i < Buckets; ++i)
        {
            acc += count[i];
            if (acc >= target) return Upper(i) / gCyclesPerNs;
        }
        return 0;
    }
    double MaxNs() const
    {
        for (int i = Buckets - 1; i >= 0; --i)
            if (count[i]) return Upper(i) / gCyclesPerNs;
        return 0;
    }
};

struct Common
{
    alignas(64) std::atomic<long long> pending{0};
    alignas(64) std::atomic<long long> processed{0};
    int consumeNs = 0;

    void Drain()
    {
        long long n = pending.exchange(0);
        for (long long i = 0; i < n; ++i) Work(consumeNs);
        processed.fetch_add(n, std::memory_order_relaxed);
    }
};

struct CasImpl : Common
{
    enum : unsigned { Busy = 1, Dirty = 2 };
    alignas(64) std::atomic<unsigned> state{0};

    bool Notify()
    {
        auto s = state.load();
        for (;;)
        {
            const bool schedule = (s & Busy) == 0;
            const unsigned next = s | Dirty | (schedule ? Busy : 0u);
            if (state.compare_exchange_weak(s, next)) return schedule;
        }
    }
    void Consume()
    {
        for (;;)
        {
            auto s = state.load();
            if (s & Dirty)
            {
                if (!state.compare_exchange_weak(s, s & ~Dirty)) continue;
                Drain();
                continue;
            }
            if (state.compare_exchange_weak(s, s & ~Busy)) return;
        }
    }
};

struct CasFastImpl : CasImpl
{
    bool Notify()
    {
        const auto s = state.load();
        if ((s & (Busy | Dirty)) == (Busy | Dirty)) return false;
        return CasImpl::Notify();
    }
};

struct LockFlagImpl : Common
{
    std::mutex m;
    bool busy = false, dirty = false;

    bool Notify()
    {
        std::lock_guard g(m);
        dirty = true;
        if (busy) return false;
        busy = true;
        return true;
    }
    void Consume()
    {
        for (;;)
        {
            {
                std::lock_guard g(m);
                if (!dirty) { busy = false; return; }
                dirty = false;
            }
            Drain();
        }
    }
};

struct CoarseImpl : Common
{
    std::mutex m;
    bool Notify() { m.lock(); return true; }
    void Consume() { Drain(); m.unlock(); }
};

template <typename Impl>
void Run(const char* name, int threads, int consumeNs, int otherNs, long long perThread)
{
    Impl impl;
    impl.consumeNs = consumeNs;
    std::vector<Histogram> hist(threads);
    std::vector<double> finishMs(threads);
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::chrono::steady_clock::time_point startTp;
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t)
        ts.emplace_back([&, t] {
            auto& h = hist[t];
            ready.fetch_add(1);
            while (!go.load()) std::this_thread::yield();
            for (long long i = 0; i < perThread; ++i)
            {
                impl.pending.fetch_add(1);
                _mm_lfence();
                const auto c0 = __rdtsc();
                const bool won = impl.Notify();
                _mm_lfence();
                const auto c1 = __rdtsc();
                h.Add(c1 - c0);
                if (won) impl.Consume();
                Work(otherNs);
            }
            finishMs[t] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTp).count();
        });
    while (ready.load() != threads) std::this_thread::yield();
    const auto begin = std::chrono::steady_clock::now();
    startTp = begin;
    go.store(true);
    for (auto& t : ts) t.join();
    const auto end = std::chrono::steady_clock::now();

    if (std::getenv("SPREAD"))
    {
        auto f = finishMs;
        std::sort(f.begin(), f.end());
        std::printf("   finish ms: min %.0f median %.0f max %.0f\n", f.front(), f[f.size() / 2], f.back());
    }
    Histogram all;
    for (auto& h : hist) all.Merge(h);
    const long long total = perThread * threads;
    const double sec = std::chrono::duration<double>(end - begin).count();
    std::printf("%-9s thr=%2d consume=%4d other=%4d | %7.1f ns/op | p50 %7.0f  p99 %8.0f  p99.9 %9.0f  p99.99 %9.0f  max %10.0f ns | lost=%lld\n",
                name, threads, consumeNs, otherNs, sec * 1e9 / total,
                all.PercentileNs(0.5), all.PercentileNs(0.99), all.PercentileNs(0.999),
                all.PercentileNs(0.9999), all.MaxNs(), total - impl.processed.load() - impl.pending.load());
}

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        Calibrate();
        for (int threads : {4, 16, 64})
        {
            Run<CasImpl>("cas", threads, 1000, 1000, 1'100'000 / threads);
            Run<CasFastImpl>("casfast", threads, 1000, 1000, 1'100'000 / threads);
            Run<LockFlagImpl>("lockflag", threads, 1000, 1000, 1'100'000 / threads);
        }
        return 0;
    }
    if (std::getenv("TRACY_SCN"))
    {
        // send executor tracy(server_crop.tracy, 18스레드, 10초) 기반 시나리오
        // 소비 13300ns = RealSend 합 / SendPacket 수, 사이 3900ns = 바쁜 시간 중 나머지, 75000ns = 유휴 포함 벽시계
        Calibrate();
        std::printf("iters/ns=%.3f cycles/ns=%.3f\n", gItersPerNs, gCyclesPerNs);
        struct Scn { int consume, other; };
        const Scn scns[] = {{13300, 3900}, {13300, 75000}};
        for (auto sc : scns)
            for (int threads : {2, 4, 8, 18})
            {
                const long long n = 100'000 / threads;
                Run<CasImpl>("cas", threads, sc.consume, sc.other, n);
                Run<CasFastImpl>("casfast", threads, sc.consume, sc.other, n);
                Run<LockFlagImpl>("lockflag", threads, sc.consume, sc.other, n);
            }
        return 0;
    }
    Calibrate();
    std::printf("iters/ns=%.3f cycles/ns=%.3f logical cpus=%u\n", gItersPerNs, gCyclesPerNs,
                std::thread::hardware_concurrency());
    // {소비 작업 ns/건, 알림 사이 다른 작업 ns, 총 알림 수}
    struct Scenario { int consume, other; long long total; };
    const Scenario scenarios[] = {{0, 0, 32'000'000}, {200, 1000, 3'200'000}, {1000, 1000, 1'600'000}};
    for (auto sc : scenarios)
        for (int threads : {4, 8, 16, 32, 64})
        {
            const long long n = sc.total / threads;
            Run<CasImpl>("cas", threads, sc.consume, sc.other, n);
            Run<CasFastImpl>("casfast", threads, sc.consume, sc.other, n);
            Run<LockFlagImpl>("lockflag", threads, sc.consume, sc.other, n);
            Run<CoarseImpl>("coarse", threads, sc.consume, sc.other, n);
        }
}
