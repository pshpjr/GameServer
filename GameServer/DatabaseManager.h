/*
 *
 * 추후 분산된 데이터베이스 처리를 한 클래스로 몰아넣기 위한 클래스
 *
 *
 */


// //
// // Created by pshpj on 24. 12. 19.
// //
//
// #ifndef DATABASEHANDLER_H
// #define DATABASEHANDLER_H
//
// #include <boost/asio.hpp>
// #include <boost/mysql.hpp>
// #include <chrono>
// #include <format>
// #include <fstream>
// #include <future>
// #include <iostream>
// #include <mutex>
// #include <string>
// #include <thread>
// #include <utility>
// #include <vector>
//
// namespace asio = boost::asio;
// using namespace boost::asio;
//
// // 데이터베이스 접속 정보를 저장할 구조체
// struct DatabaseConfig {
//     //접속 url
//     std::string host;
//     std::string user;
//     std::string password;
//     std::string database;
//     int port;
// };
//
// class
// DatabaseManager {
// public:
//     DatabaseManager(int num_threads, int num_connections,
//                     DatabaseConfig db_config);
//
//     int run();
//
//     void stop();
//
//     void query(const std::string& query);
//
//     void query(const std::string& query, std::function<void(std::string)> callback);
// private:
//     const int _numThreads;
//     const int _numConnections;
//
//     DatabaseConfig _dbConfig;  // DB 접속 정보 구성
//
//     std::unique_ptr<io_context> _ioContextPtr;
//     std::vector<std::jthread> _ioThreads;
//     std::unique_ptr<boost::mysql::connection_pool> _pool;
//     awaitable<void> coroutineWorker();
//
//     void startIoContextThreads();
// };
//
// #endif //DATABASEHANDLER_H