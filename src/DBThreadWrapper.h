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
    public:
        struct MonitorData
        {
            int64 queued = 0;
            int64 enqueue = 0;
            int64 dequeue = 0;
            int64 delaySum = 0;
        };

        DBThreadWrapper(LPCSTR IP, Port port, LPCSTR ID, LPCSTR PWD, LPCSTR Schema)
            : conn(IP, port, ID, PWD, Schema)
            , dbThread(&DBThreadWrapper::DBWorkerFunc, this)
            , _queue(std::make_unique<LockFreeFixedQueue<std::function<void()>, 1024>>())
        {
        }


        void UpdateCoin(const std::shared_ptr<DBData>& dbData);
        void UpdateLocation(const std::shared_ptr<DBData>& dbData);
        void UpdateHP(const std::shared_ptr<DBData>& dbData);
        void EnterGroup(const std::shared_ptr<DBData>& dbData);
        void LeaveGroup(const std::shared_ptr<DBData>& dbData, Server* server, SessionID session, GroupID nextGroup);


        //getMonitor 사이의 값을 출력. 
        MonitorData GetMonitor()
        {
            MonitorData data;
            data.queued = _queue->Size();
            data.enqueue = _oldData.enqueue.exchange(0);
            data.dequeue = _oldData.dequeue.exchange(0);
            data.delaySum = _oldData.delaySum.exchange(0);
            return data;
        }

    private:
        struct atomicMonitorData
        {
            std::atomic<int64> queued = 0;
            std::atomic<int64> enqueue = 0;
            std::atomic<int64> dequeue = 0;
            std::atomic<int64> delaySum = 0;
        };

        void Enqueue(const std::function<void()>& func);
        void DBWorkerFunc();

        MonitorData _data;
        atomicMonitorData _oldData;

        DBConnection conn;
        std::jthread dbThread;

        std::atomic<bool> hasData = false;
        std::unique_ptr<LockFreeFixedQueue<std::function<void()>, 1024>> _queue;
    };
}
