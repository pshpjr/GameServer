// ReSharper disable CppParameterMayBeConst
#pragma once

#include <future>
#include <memory>
#include <thread>

#include "DBConnection.h"
#include "GroupTypes.h"
#include "Metrics.h"
#include "SPSCQueue.h"
#include "SocketTypes.h"

class Logger;

namespace psh
{
    class Server;
    class DBData;
    class Player;

    //비동기 쿼리 처리를 해주는 스레드 래핑.
    class DBThreadWrapper
    {
        using jobQ = SPSCQueue<std::function<void()>, 1024>;
        struct DbTask;
    public:
        DBThreadWrapper(jobQ& compAlert, LPCSTR IP, Port port, LPCSTR ID, LPCSTR PWD, LPCSTR Schema);

        ~DBThreadWrapper();

        void Execute(DbTask task);

        void UpdateCoin(const std::shared_ptr<DBData>& dbData);
        void UpdateLocation(const std::shared_ptr<DBData>& dbData);
        void UpdateHP(const std::shared_ptr<DBData>& dbData);
        void EnterGroup(const std::shared_ptr<DBData>& dbData);

        //callback은 완료 후 compAlert큐에 저장됨.
        void SaveAll(std::shared_ptr<DBData> dbData, std::function<void()> callback);

    private:
        void Enqueue(const std::function<void()>& func);
        void DBWorkerFunc();

        //대부분 읽기만 함.
        std::unique_ptr<Logger> _logger = nullptr;
        std::atomic<bool> dbThreadRunning{true};
        std::unique_ptr<jobQ> _jobQueue;
        DBConnection _conn;
        jobQ& _completeAlert;

        //  DB 스레드가 여러 개여도 같은 시리즈에 합산된다.
        metrics::Histogram _queryMetric = metrics::Registry::CreateTimeHistogram("db.query.duration", "query time");
        metrics::Histogram _queueWaitMetric = metrics::Registry::CreateTimeHistogram(
            "db.queue.wait.duration", "time from Execute until the DB thread starts the query");
        metrics::Counter _queryOkMetric = metrics::Registry::CreateCounter("db.queries", "executed queries", {{"result", "ok"}});
        metrics::Counter _queryErrorMetric = metrics::Registry::CreateCounter("db.queries", "executed queries", {{"result", "error"}});
        metrics::Gauge _queueMetric = metrics::Registry::CreateGauge("db.queue.size", "queued DB jobs");

        std::jthread dbThread;
        //여기서 새 캐시라인. 항상 오프셋 확인할 것.

        //hasData는 쓰기 경합이 많으니 따로 두자.
        alignas(64) std::atomic<bool> hasData = false;
    };
}
