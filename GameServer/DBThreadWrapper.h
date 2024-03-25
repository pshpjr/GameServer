#pragma once
#include <condition_variable>
#include <future>
#include <memory>
#include <memory>

#include "LockFreeFixedQueue.h"
#include "DBConnection.h"


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
        
        DBThreadWrapper(LPCSTR IP, Port port, LPCSTR ID, LPCSTR PWD, LPCSTR Schema) : conn(IP,port,ID,PWD,Schema)
        , dbThread(&DBThreadWrapper::DBWorkerFunc,this)
        , _queue(make_unique<LockFreeFixedQueue<function<void()>, 1024>>())
        {
        
        }

        
        void UpdateCoin(const shared_ptr<DBData>& dbData);
        void UpdateLocation(const shared_ptr<DBData>& dbData);
        void UpdateHP(const shared_ptr<DBData>& dbData);
        void EnterGroup(const shared_ptr<DBData>& dbData);
        void LeaveGroup(const shared_ptr<DBData>& dbData,Server* server, SessionID session, GroupID nextGroup);

        
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
            atomic<int64> queued = 0;
            atomic<int64> enqueue = 0;
            atomic<int64> dequeue = 0;
            atomic<int64> delaySum = 0;
        };
        
        void Enqueue(const function<void()>& func);
        void DBWorkerFunc();
        MonitorData _data;
        atomicMonitorData _oldData;

        DBConnection conn;
        std::jthread dbThread;

        atomic<bool> hasData = false;
        unique_ptr<LockFreeFixedQueue<function<void()>, 1024>> _queue;
    };

}
