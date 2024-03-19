// #include "DBThreadWrapper.h"
//
// #include "Macro.h"
//
// void DBThreadWrapper::Enqueue(const function<void()>& func)
// {
//     _queue->Enqueue(func);
// }
//
// void DBThreadWrapper::DBWorkerFunc()
// {
//     while (true) {
//         function<void()> task;
//         {
//             // Critical section
//             cv.wait(queueWait, [this] { return _queue->Size() != 0; });  // Wait until the queue is not empty
//             
//             auto deqResult = _queue->Dequeue(task);
//             ASSERT_CRASH(deqResult);
//             data.handled++;
//         } // End of critical section
//
//         task();  // Execute the task
//     }
// }
