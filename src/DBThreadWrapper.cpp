#include "DBThreadWrapper.h"

#include "Player.h"
#include "Server.h"

namespace psh
{
    void DBThreadWrapper::UpdateCoin(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([this, dbData]
        {
            const auto time = conn.Query(
                                         "UPDATE `mydb`.`player` SET `Coins` = '%d' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                                       , dbData->Coin(), dbData->AccountNo());
            _data.delaySum += time.count();
            conn.reset();
        });
    }

    void DBThreadWrapper::UpdateLocation(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([this, dbData]
        {
            const auto time = conn.Query(
                                         "UPDATE `mydb`.`player` SET `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                                       , dbData->Location().X, dbData->Location().Y, dbData->AccountNo());

            _data.delaySum += time.count();
            conn.reset();
        });
    }

    void DBThreadWrapper::UpdateHP(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([dbData,this]
        {
            const auto time = conn.Query(
                                         "UPDATE `mydb`.`player` SET `HP` = '%d' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                                       , dbData->Hp(), dbData->AccountNo());
            _data.delaySum += time.count();
            conn.reset();
        });
    }

    void DBThreadWrapper::EnterGroup(const std::shared_ptr<DBData>& dbData)
    {
        Enqueue([dbData, this]
        {
            const auto time = conn.Query(
                                         "UPDATE `mydb`.`player` SET `ServerType` = '%d', `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                                       , dbData->ServerType(), dbData->Location().X, dbData->Location().Y, dbData->AccountNo());
            _data.delaySum += time.count();

            conn.reset();
        });
    }

    void DBThreadWrapper::LeaveGroup(const std::shared_ptr<DBData>& dbData
        , Server* server
        , SessionID session
        , GroupID nextGroup)
    {
        Enqueue([dbData,this,server,session,nextGroup]
        {
            const auto time = conn.Query(
                                         "UPDATE `mydb`.`player` SET `HP` = '%d', `ServerType` = '%d', `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                                       , dbData->Hp(), dbData->ServerType(), dbData->Location().X, dbData->Location().Y, dbData->AccountNo());
            _data.delaySum += time.count();
            //auto time = conn.Query("SELECT * from `mydb`.`player` WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
            //    , dbData->AccountNo());

            _data.delaySum += 1;
            conn.reset();
            server->MoveSession(session, nextGroup);
        });
        //server->MoveSession(session,nextGroup);
    }

    void DBThreadWrapper::Enqueue(const std::function<void()>& func)
    {
        _queue->Enqueue(func);

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
        while (true)
        {
            hasData.wait(false, std::memory_order::memory_order_acquire);

            std::function<void()> task;
            while (_queue->Dequeue(task))
            {
                task();

                _data.dequeue++;
                if (_data.dequeue >= 10)
                {
                    _oldData.dequeue += 10;
                    _oldData.delaySum += _data.delaySum;

                    _data.dequeue = 0;
                    _data.delaySum = 0;
                }
            }

            hasData.store(false);
        }
    }
}
