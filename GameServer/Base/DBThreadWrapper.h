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
            int queued = 0;
            int querysPerUpdate = 0;
        
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
        MonitorData GetMonitor() const{return _data;}
   
    private:
        void Enqueue(const function<void()>& func);
        void DBWorkerFunc();
        MonitorData _data;

        DBConnection conn;
        std::jthread dbThread;

        atomic<bool> hasData = false;
        unique_ptr<LockFreeFixedQueue<function<void()>, 1024>> _queue;
    };

}
