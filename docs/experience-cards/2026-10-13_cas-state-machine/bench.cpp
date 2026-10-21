// "알림 → 단일 소비자 실행" 패턴의 구현 3종 비교.
// 생산자: pending++ 후 RequestHandling(). 실행권을 얻은 스레드가 그 자리에서 pending을 비운다.
//  - cas      : Busy|Dirty 비트를 CAS 루프로 전환 (Session::_recvState 방식)
//  - casfast  : cas + 이미 예약돼 있으면 load만 하고 반환 (Group::RequestUpdate 방식)
//  - lockflag : 같은 Busy/Dirty 플래그를 mutex로 보호, 소비는 락 밖에서
//  - coarse   : 소비 전체를 mutex 안에서 수행 (흔한 락 기반 구현)
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

static double gItersPerNs = 1.0;
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
    const long long n = 200'000'000;
    const auto b = std::chrono::steady_clock::now();
    Spin(n);
    const auto e = std::chrono::steady_clock::now();
    gItersPerNs = n / double(std::chrono::duration_cast<std::chrono::nanoseconds>(e - b).count());
}

struct Common
{
    alignas(64) std::atomic<long long> pending{0};
    alignas(64) std::atomic<long long> processed{0};
    alignas(64) std::atomic<long long> passes{0};
    int workIters = 0;

    void Drain()
    {
        passes.fetch_add(1, std::memory_order_relaxed);
        long long n = pending.exchange(0);
        for (long long i = 0; i < n; ++i) Work(workIters);
        processed.fetch_add(n, std::memory_order_relaxed);
    }
};

struct CasImpl : Common
{
    enum : unsigned { Busy = 1, Dirty = 2 };
    alignas(64) std::atomic<unsigned> state{0};

    void Request()
    {
        auto s = state.load();
        for (;;)
        {
            const bool schedule = (s & Busy) == 0;
            const unsigned next = s | Dirty | (schedule ? Busy : 0u);
            if (state.compare_exchange_weak(s, next)) { if (schedule) Consume(); return; }
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

// Group::RequestUpdate처럼 이미 "한 번 더"가 예약돼 있으면 RMW 없이 load만 하고 나간다.
struct CasFastImpl : CasImpl
{
    void Request()
    {
        auto s = state.load();
        if ((s & (Busy | Dirty)) == (Busy | Dirty)) return;
        CasImpl::Request();
    }
};

struct LockFlagImpl : Common
{
    std::mutex m;
    bool busy = false, dirty = false;

    void Request()
    {
        {
            std::lock_guard g(m);
            dirty = true;
            if (busy) return;
            busy = true;
        }
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
    void Request()
    {
        std::lock_guard g(m);
        Drain();
    }
};

template <typename Impl>
void Run(const char* name, int threads, int workIters, int otherNs, long long perThread)
{
    Impl impl;
    impl.workIters = workIters;
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t)
        ts.emplace_back([&] {
            ready.fetch_add(1);
            while (!go.load()) {}
            for (long long i = 0; i < perThread; ++i)
            {
                impl.pending.fetch_add(1);
                impl.Request();
                Work(otherNs);
            }
        });
    while (ready.load() != threads) {}
    const auto begin = std::chrono::steady_clock::now();
    go.store(true);
    for (auto& t : ts) t.join();
    const auto end = std::chrono::steady_clock::now();

    const long long total = perThread * threads;
    const double sec = std::chrono::duration<double>(end - begin).count();
    std::printf("%-9s threads=%2d consume=%4dns other=%4dns  %8.2f Mops/s  %7.1f ns/op  passes/op=%.3f  leftover=%lld lost=%lld\n",
                name, threads, workIters, otherNs, total / sec / 1e6, sec * 1e9 / total,
                double(impl.passes.load()) / total, impl.pending.load(), total - impl.processed.load());
}

int main(int argc, char** argv)
{
    Calibrate();
    std::printf("iters/ns=%.3f\n", gItersPerNs);
    const long long perThread = argc > 1 ? std::stoll(argv[1]) : 2'000'000;
    // {소비 작업 ns/건, 알림 사이 다른 작업 ns}
    // {소비 작업 ns/건, 알림 사이 다른 작업 ns}
    const int scenarios[][2] = {{0, 0}, {200, 0}, {200, 1000}, {1000, 1000}};
    for (auto [consume, other] : scenarios)
        for (int threads : {1, 2, 3, 4, 16})
        {
            const long long n = (consume || other) ? perThread / 20 : perThread;
            Run<CasImpl>("cas", threads, consume, other, n);
            Run<CasFastImpl>("casfast", threads, consume, other, n);
            Run<LockFlagImpl>("lockflag", threads, consume, other, n);
            Run<CoarseImpl>("coarse", threads, consume, other, n);
        }
}
