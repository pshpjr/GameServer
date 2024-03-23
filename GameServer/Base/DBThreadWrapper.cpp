#include "DBThreadWrapper.h"

#include "Macro.h"
#include "Player.h"
#include "../Server.h"

namespace psh
{
    
    void DBThreadWrapper::UpdateCoin(const shared_ptr<DBData>& dbData)
    {
        Enqueue([dbData,this]()
        {
            conn.Query("UPDATE `mydb`.`player` SET `Coins` = '%d' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');",dbData->Coin(),dbData->AccountNo());
            conn.reset();
        });
    }

    void DBThreadWrapper::UpdateLocation(const shared_ptr<DBData>& dbData)
    {
        Enqueue([dbData,this]()
        {
            conn.Query("UPDATE `mydb`.`player` SET `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                ,dbData->Location().X,dbData->Location().Y,dbData->AccountNo());
            conn.reset();
        });
    }

    void DBThreadWrapper::UpdateHP(const shared_ptr<DBData>& dbData)
    {
        Enqueue([dbData,this]()
        {
            conn.Query("UPDATE `mydb`.`player` SET `HP` = '%d' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
                ,dbData->Hp(),dbData->AccountNo());
            conn.reset();
        });
    }

    void DBThreadWrapper::EnterGroup(const shared_ptr<DBData>& dbData)
    {
        conn.Query("UPDATE `mydb`.`player` SET `ServerType` = '%d', `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
            ,dbData->ServerType(),dbData->Location().X,dbData->Location().Y,dbData->AccountNo());
        conn.reset();
    }

    void DBThreadWrapper::LeaveGroup(const shared_ptr<DBData>& dbData,Server* server, SessionID session, GroupID nextGroup)
    {

        Enqueue([dbData,this,server,session,nextGroup]()
        {
            conn.Query("UPDATE `mydb`.`player` SET `HP` = '%d', `ServerType` = '%d', `LocationX` = '%f', `LocationY` = '%f' WHERE (`PlayerId` = '0') and (`AccountNo` = '%lld');"
           ,dbData->Hp(),dbData->ServerType(),dbData->Location().X,dbData->Location().Y,dbData->AccountNo());

            server->MoveSession(session,nextGroup);
            conn.reset();
        });
        
    }

    void DBThreadWrapper::Enqueue(const function<void()>& func)
    {
        _queue->Enqueue(func);
        hasData.store(true,memory_order::memory_order_release);
        hasData.notify_one();
    }

    void DBThreadWrapper::DBWorkerFunc()
    {
        while (true) {
            function<void()> task;
        
            hasData.wait(false,memory_order::memory_order_acquire);
        
            auto deqResult = _queue->Dequeue(task);
            ASSERT_CRASH(deqResult);
        
            task();

            hasData.store(false);
        }
    }

}
