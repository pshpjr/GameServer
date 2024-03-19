// #pragma once
// #include <condition_variable>
// #include <future>
//
// #include "LockFreeFixedQueue.h"
// #include "DBConnection.h"
//
// class DBThreadWrapper
// {
// public:
//     struct MonitorData
//     {
//         int queued = 0;
//         int querysPerUpdate = 0;
//     };
//     
//     DBThreadWrapper() : conn("10.0.2.2", 3306, "root", "12321", "test")
//     , dbThread(&DBThreadWrapper::DBWorkerFunc,this)
//     //, queueWait(jobQueueMutex)
//     , _queue(make_unique<LockFreeFixedQueue<function<void()>, 1024>>())
//     {
//         
//     }
//
//     void Enqueue(const function<void()>& func);
//     
//     MonitorData GetMonitor() const{return _data;}
//    
//     void DBWorkerFunc();
//     
// private:
//     MonitorData _data;
//     std::condition_variable cv;
//     DBConnection conn;
//     std::jthread dbThread;
//     
//     std::mutex jobQueueMutex;
//     condition_variable queueWait;
//     unique_ptr<LockFreeFixedQueue<function<void()>, 1024>> _queue;
//
//
// };
