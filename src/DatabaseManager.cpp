// //
// // Created by pshpj on 24. 12. 19.
// //
//
// #include "DatabaseManager.h"
// DatabaseManager::    DatabaseManager(int num_threads, int num_connections,
//                     DatabaseConfig db_config)
//     : _numThreads(num_threads),
//       _numConnections(num_connections),
//       _dbConfig(std::move(db_config)),
//       _ioContextPtr{std::make_unique<io_context>(num_threads)} {}
//
// int DatabaseManager::run() {
//     // 코루틴 생성 및 시작
//     for (int i = 0; i < _numThreads; ++i) {
//         co_spawn(*_ioContextPtr, coroutineWorker(std::move(p), i),
//                  detached);
//     }
//
//     startIoContextThreads();
//
//     return 0;
// }
// void DatabaseManager::stop() {
// }
//
// awaitable<void> DatabaseManager::coroutineWorker() {
//     auto executor = co_await asio::this_coro::executor;
//
//     // MySQL 연결 생성
//     boost::mysql::tcp_connection conn(executor);
//
//     // MySQL 연결 파라미터
//     boost::mysql::handshake_params params(_dbConfig.user, _dbConfig.password,
//                                           _dbConfig.database);
//
//     // DNS 해석기
//     ip::tcp::resolver resolver(executor);
//
//     try {
//         // 호스트 해석
//         auto endpoints = co_await resolver.async_resolve(
//             _dbConfig.host, std::to_string(_dbConfig.port), use_awaitable);
//
//         // 서버에 연결
//         co_await conn.async_connect(*endpoints.begin(), params, use_awaitable);
//     } catch (const std::exception& e) {
//
//         co_return;
//     }
//
//     while (true) {
//
//     }
//
//     // 시간 측정 시작
//     auto start_time = std::chrono::high_resolution_clock::now();
//
//     // 쿼리 실행
//     for (int i = 0; i < _numQueries; ++i) {
//         std::string query;
//         if (i % 2) {
//             query = std::format("SELECT * FROM `test`.`perf_test` WHERE `id` = {};",
//                                 queryStart + i);
//         } else {
//             query = std::format(
//                 "UPDATE `test`.`perf_test` SET `value`= 2 WHERE `id` = {};",
//                 queryStart + i);
//         }
//
//         try {
//
//         } catch (const std::exception& e) {
//             std::lock_guard<std::mutex> lock(_coutMutex);
//             std::cerr << "Coroutine " << coroutine_id
//                       << ": Query failed: " << e.what() << "\n";
//             break;
//         }
//     }
//
//     // 시간 측정 종료
//     auto end_time = std::chrono::high_resolution_clock::now();
//     std::chrono::duration<double> elapsed = end_time - start_time;
//
//     p.set_value(elapsed);
//
//     // 연결 종료
//     try {
//         co_await conn.async_close(use_awaitable);
//     } catch (...) {
//         // 연결 종료 중 예외는 무시
//     }
// }
// void DatabaseManager::startIoContextThreads() {
//     for (int i = 0; i < _numCorutineThread; ++i) {
//         _ioThreads.emplace_back([this]() { _ioContextPtr->run(); });
//     }
// }
// void DatabaseManager::saveResults(
//     std::vector<std::future<std::chrono::duration<double>>>& coroutine_times) {
//     std::string filename =
//         std::format("Coroutine_{}c_{}q_{}t_{}.txt", _numThreads, _numQueries,
//                     _numCorutineThread, Utils::DateStr());
//
//     std::ofstream fs(filename);
//     fs << "\n=== Coroutine Normal Approach ===\n";
//
//     for (int i = 0; i < coroutine_times.size(); ++i) {
//         auto duration = coroutine_times[i].get();
//         if (duration.count() > 0)
//             fs << "Coroutine " << i << ": " << duration.count() << " seconds\n";
//         else
//             fs << "Coroutine " << i << ": Failed to execute queries.\n";
//     }
//   }