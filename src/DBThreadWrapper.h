// ReSharper disable CppParameterMayBeConst
#pragma once
#include <future>
#include <memory>
#include <memory>
#include "SessionTypes.h"

#include "LockFreeFixedQueue.h"

#include "DBConnection.h"
#include "GroupTypes.h"
#include "SocketTypes.h"

namespace psh
{
    class Server;
    class DBData;
    class Player;

    class DBThreadWrapper
    {
        using jobQ = LockFreeFixedQueue<std::function<void()>, 1024>;

    public:
        struct MonitorData
        {
            int64 queued = 0;
            int64 enqueue = 0;
            int64 dequeue = 0;
            std::chrono::microseconds delaySum = std::chrono::microseconds::zero();
            int32 err = 0;
        };

        DBThreadWrapper(jobQ& compAlert, LPCSTR IP, Port port, LPCSTR ID, LPCSTR PWD, LPCSTR Schema)
            : conn(IP, port, ID, PWD, Schema)
            , dbThread(&DBThreadWrapper::DBWorkerFunc, this)
            , _jobQueue(std::make_unique<LockFreeFixedQueue<std::function<void()>, 1024>>())
            , _completeAlert{compAlert} {}

        ~DBThreadWrapper();

        void UpdateCoin(const std::shared_ptr<DBData>& dbData);
        void UpdateLocation(const std::shared_ptr<DBData>& dbData);
        void UpdateHP(const std::shared_ptr<DBData>& dbData);
        void EnterGroup(const std::shared_ptr<DBData>& dbData);

        //callback은 완료 후 compAlert큐에 저장됨.
        void SaveAll(std::shared_ptr<DBData> dbData, std::function<void()> callback);


        //getMonitor 사이의 값을 출력. 
        MonitorData GetMonitor()
        {
            MonitorData data;
            data.queued = _jobQueue->Size();
            data.enqueue = _oldData.enqueue.exchange(0);
            data.dequeue = _oldData.dequeue.exchange(0);
            data.delaySum = std::chrono::milliseconds(_oldData.delaySum.exchange(0));
            data.err = _oldData.err.exchange(0);
            return data;
        }

    private:
        struct atomicMonitorData
        {
            std::atomic<int64> queued = 0;
            std::atomic<int64> enqueue = 0;
            std::atomic<int64> dequeue = 0;
            std::atomic<int64> delaySum{};
            std::atomic<int32> err = 0;
        };

        void Enqueue(const std::function<void()>& func);
        void DBWorkerFunc();

        //대부분 읽기만 함.
        std::atomic<bool> dbThreadRunning{true};
        std::unique_ptr<jobQ> _jobQueue;
        DBConnection conn;
        jobQ& _completeAlert;
        MonitorData _data;

        //여기서 새 캐시라인. 항상 오프셋 확인할 것.

        //hasData, _oldData는 쓰기 경합이 많으니 따로 두자.
        atomicMonitorData _oldData;
        std::jthread dbThread;

        //oldData 수정시 경합 적게 따로 둠.
        alignas(64) std::atomic<bool> hasData = false;
    };
}
