#include "DBThreadWrapper.h"
#include "Player.h"
#include "Server.h"
#include <exception>

#include "optick.h"


/*
 *
 * MYSQL 성능 테스트 해 봤을 때 저장 프로시저가 더 느림
 * UPDATE, SELECT 반반 섞어 진행했고, 6커넥션 66667 쿼리 기준
 * 30.6251s, 32.0638s 차이. 한 5%?
 * 너무 단순한 쿼리라 저장 프로시저 호출에 들어가는 오버헤드가 더 큰 듯
 * 로직 분리의 이유가 없다면 지금은 필요 없다.
 *
 */

namespace psh
{
    namespace
    {
        constexpr const char* DB_NAME = "mydb";
        constexpr const char* PLAYER_TABLE = "player";
        constexpr int SLOW_QUERY_THRESHOLD_MS = 500;
    }


    DBThreadWrapper::DBThreadWrapper(jobQ& compAlert, LPCSTR IP, Port port, LPCSTR ID, LPCSTR PWD, LPCSTR Schema):
        _logger(std::make_unique<CLogger>(L"DBThreadWrapper.txt"))
        , _jobQueue(std::make_unique<jobQ>())
        , _conn(IP, port, ID, PWD, Schema)
        , _completeAlert{compAlert}
        , dbThread(&DBThreadWrapper::DBWorkerFunc, this)
    {
    }

    DBThreadWrapper::~DBThreadWrapper()
    {
        dbThreadRunning.store(false, std::memory_order_release);
        hasData.store(true, std::memory_order_release);
        hasData.notify_one();
    }

    struct DBThreadWrapper::DbTask
    {
        std::string description;
        std::string query;
        std::function<void()> callback;

        DbTask(std::string desc, std::string q, std::function<void(void)> cb) : description(std::move(desc)),
            query(std::move(q)), callback(std::move(cb))
        {
        }
    };

    void DBThreadWrapper::Execute(DbTask task)
    {
        Enqueue([this, task = std::move(task)]()
        {
            try
            {
                auto queryTime = _conn.QueryFormat(task.query);

                if (queryTime.count() > SLOW_QUERY_THRESHOLD_MS)
                {
                    // 멀티스레드 환경이고, 파일 접근 동시에 할 수 있음.
                    _logger->Write(L"Slow", CLogger::LogLevel::Debug, L"%S slow %lld ms, Query: %S",
                                   task.description.c_str(), queryTime.count(), task.query.c_str());
                }
                _data.delaySum += queryTime;

                if (task.callback)
                {
                    _completeAlert.Enqueue(std::move(task.callback));
                }
            }
            catch (const std::exception& e)
            {
                _logger->Write(L"Error", CLogger::LogLevel::Error, L"DB operation failed: %S, Query: %S, Error: %S",
                               task.description.c_str(), task.query.c_str(), e.what());
                auto exceptionPtr = std::current_exception();
                _completeAlert.Enqueue([exceptionPtr]
                {
                    std::rethrow_exception(exceptionPtr);
                });
            }
            // RAII 방식으로 연결 관리가 되면 여기서 reset() 불필요
            _conn.reset();
        });
    }

    void DBThreadWrapper::UpdateCoin(const std::shared_ptr<DBData>& dbData)
    {
        Execute(DbTask{
            "UpdateCoin",
            std::format("UPDATE `{}`.`{}` SET `Coins` = {} WHERE (`PlayerId` = 0) and (`AccountNo` = {});",
                        DB_NAME, PLAYER_TABLE, dbData->Coin(), dbData->AccountNum()),
            {}
        });
    }

    void DBThreadWrapper::UpdateLocation(const std::shared_ptr<DBData>& dbData)
    {
        Execute(DbTask(
            "UpdateLocation",
            std::format(
                "UPDATE `{}`.`{}` SET `LocationX` = {}, `LocationY` = {} WHERE (`PlayerId` = 0) and (`AccountNo` = {});",
                DB_NAME, PLAYER_TABLE, dbData->Location().X, dbData->Location().Y, dbData->AccountNum()),
            {}));
    }

    void DBThreadWrapper::UpdateHP(const std::shared_ptr<DBData>& dbData)
    {
        Execute(DbTask(
            "UpdateHP",
            std::format("UPDATE `{}`.`{}` SET `HP` = {} WHERE (`PlayerId` = 0) and (`AccountNo` = {});",
                        DB_NAME, PLAYER_TABLE, dbData->Hp(), dbData->AccountNum()),
            {}));
    }

    void DBThreadWrapper::EnterGroup(const std::shared_ptr<DBData>& dbData)
    {
        Execute(DbTask(
            "EnterGroup",
            std::format(
                "UPDATE `{}`.`{}` SET `ServerType` = {}, `LocationX` = {}, `LocationY` = {} WHERE (`PlayerId` = 0) and (`AccountNo` = {});",
                DB_NAME, PLAYER_TABLE, static_cast<int>(dbData->ServerType()), dbData->Location().X,
                dbData->Location().Y, dbData->AccountNum()),
            {}));
    }

    void DBThreadWrapper::SaveAll(std::shared_ptr<DBData> dbData, std::function<void()> callback)
    {
        Execute(DbTask(
            "SaveAll",
            std::format(
                "UPDATE `{}`.`{}` SET `HP` = {}, `ServerType` = {}, `LocationX` = {}, `LocationY` = {} WHERE (`PlayerId` = 0) and (`AccountNo` = {});",
                DB_NAME, PLAYER_TABLE, dbData->Hp(), static_cast<int>(dbData->ServerType()), dbData->Location().X,
                dbData->Location().Y, dbData->AccountNum()),
            callback));
    }


    void DBThreadWrapper::Enqueue(const std::function<void()>& func)
    {
        if (dbThreadRunning.load(std::memory_order::memory_order_acquire) == false)
        {
            ASSERT_CRASH(false, "Enqueue when DB end");
        }

        OPTICK_EVENT()
        while (_jobQueue->Enqueue(func) == false)
        {
            _logger->Write(L"DBThread", CLogger::LogLevel::System, L"DBThreadWrapper::Enqueue failed");
            std::this_thread::yield();
        }

        _data.enqueue++;
        if (_data.enqueue >= 10)
        {
            _oldData.enqueue += 10;
            _data.enqueue = 0;
        }

        hasData.store(true, std::memory_order::memory_order_release);
        hasData.notify_one();
    }

    void DBThreadWrapper::DBWorkerFunc()
    {
        while (dbThreadRunning.load(std::memory_order::memory_order_acquire))
        {
            hasData.wait(false, std::memory_order::memory_order_acquire);

            std::function<void()> task;
            while (_jobQueue->Dequeue(task))
            {
                task();

                _data.dequeue++;
                if (_data.dequeue >= 10)
                {
                    _oldData.dequeue += 10;
                    _oldData.delaySum += duration_cast<std::chrono::milliseconds>(_data.delaySum).count();

                    _data.dequeue = 0;
                    _data.delaySum = std::chrono::microseconds::zero();
                }
            }

            hasData.store(false);
        }
    }
}
