#include "DBThreadWrapper.h"
#include "Player.h"
#include "Server.h"
#include <exception>

#include "optick.h"

namespace psh
{
    DBThreadWrapper::~DBThreadWrapper()
    {
        dbThreadRunning.store(false, std::memory_order_release);
        hasData.store(true, std::memory_order_release);
        hasData.notify_one();
    }

    void DBThreadWrapper::UpdateCoin(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([this, dbData] {
            try
            {
                const auto time = conn.Query(
                    "UPDATE `mydb`.`player` SET `Coins` = '%d' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                    , dbData->Coin(), dbData->AccountNum());
                _data.delaySum += time;
                conn.reset();
            }
            catch (const std::exception& e)
            {
                auto exceptionPtr = std::current_exception();
                _completeAlert.Enqueue([exceptionPtr] {
                    std::rethrow_exception(exceptionPtr);
                });
            }
        });
    }

    void DBThreadWrapper::UpdateLocation(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([this, dbData] {
            try
            {
                const auto time = conn.Query(
                    "UPDATE `mydb`.`player` SET `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                    , dbData->Location().X, dbData->Location().Y, dbData->AccountNum());
                _data.delaySum += time;
                conn.reset();
            }
            catch (const std::exception& e)
            {
                auto exceptionPtr = std::current_exception();
                _completeAlert.Enqueue([exceptionPtr] {
                    std::rethrow_exception(exceptionPtr);
                });
            }
        });
    }

    void DBThreadWrapper::UpdateHP(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([this, dbData] {
            try
            {
                const auto time = conn.Query(
                    "UPDATE `mydb`.`player` SET `HP` = '%d' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                    , dbData->Hp(), dbData->AccountNum());
                _data.delaySum += time;
                conn.reset();
            }
            catch (const std::exception& e)
            {
                auto exceptionPtr = std::current_exception();
                _completeAlert.Enqueue([exceptionPtr] {
                    std::rethrow_exception(exceptionPtr);
                });
            }
        });
    }

    void DBThreadWrapper::EnterGroup(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([this, dbData] {
            try
            {
                const auto time = conn.Query(
                    "UPDATE `mydb`.`player` SET `ServerType` = '%d', `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                    , dbData->ServerType(), dbData->Location().X, dbData->Location().Y, dbData->AccountNum());
                _data.delaySum += time;
                conn.reset();
            }
            catch (const std::exception& e)
            {
                auto exceptionPtr = std::current_exception();
                _completeAlert.Enqueue([exceptionPtr] {
                    std::rethrow_exception(exceptionPtr);
                });
            }
        });
    }

    void DBThreadWrapper::SaveAll(std::shared_ptr<DBData> dbData, std::function<void()> callback)
    {
        Enqueue([this, dbData = std::move(dbData), callback= std::move(callback)] {
            try
            {
                const auto time = conn.Query(
                    "UPDATE `mydb`.`player` SET `HP` = '%d', `ServerType` = '%d', `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                    , dbData->Hp(), dbData->ServerType(), dbData->Location().X, dbData->Location().Y
                    , dbData->AccountNum());
                _data.delaySum += time;
                conn.reset();

                if (callback)
                {
                    _completeAlert.Enqueue(callback);
                }
            }
            catch (const std::exception& e)
            {
                auto exceptionPtr = std::current_exception();
                _completeAlert.Enqueue([exceptionPtr] {
                    std::rethrow_exception(exceptionPtr);
                });
            }
        });
    }

    void DBThreadWrapper::Enqueue(const std::function<void()>& func)
    {
        if (dbThreadRunning.load(std::memory_order::memory_order_acquire) == false)
        {
            ASSERT_CRASH(false, "Enqueue when DB end");
        }

        OPTICK_EVENT()
        _jobQueue->Enqueue(func);

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
