#include "EasyMonsterGroup.h"

#include <PacketGenerated.h>

#include "FieldObjectManager.h"
#include "PveAttackManager.h"
#include "IOCP.h"
#include "Profiler.h"

namespace psh
{
    EasyMonsterGroup::EasyMonsterGroup(Server& server, const ServerInitData& data, ServerType type, short mapSize, short sectorSize):
        GroupCommon(server
            ,data
        , type
        ,mapSize, sectorSize)
        , _monsterMap(make_unique<GameMap<shared_ptr<Monster>>>(mapSize, sectorSize))
        , _itemMap(make_unique<GameMap<shared_ptr<Item>>>(mapSize, sectorSize))
    {
        _attackManager = static_cast<unique_ptr<AttackManager>>(make_unique<PveAttackManager>(*_monsterMap, *_playerMap));
        _objectManager =
            unique_ptr<ObjectManager>{
                static_cast<psh::ObjectManager*>(
                    new FieldObjectManager(*this,*_playerMap,*_monsterMap,*_itemMap,_attackManager.get())) };

    }

    void EasyMonsterGroup::OnRecv(const SessionID id, CRecvBuffer& recvBuffer)
    {
        ePacketType type;
        recvBuffer >> type;
        switch (type)
        {
            case eGame_ReqChangeComplete:
                RecvChangeComp(id, recvBuffer);
                break;
            case eGame_ReqMove:
                RecvMove(id, recvBuffer);
                break;
            case eGame_ReqAttack:
                RecvAttack(id, recvBuffer);
                break;
            case eGame_ReqLevelEnter:
                RecvReqLevelChange(id, recvBuffer);
                break;
            case eGame_reqChat:
                RecvChat(id, recvBuffer);
                break;
            default:
                DebugBreak();
                break;
        }
    }

    void EasyMonsterGroup::UpdateContent(int deltaMs)
    {
        GroupCommon::UpdateContent(deltaMs);
        auto objectManager =static_cast<FieldObjectManager*>(_objectManager.get());
        
        if(inputDelay > 0)
            inputDelay-=deltaMs;
        else
        {
            if(GetAsyncKeyState('P') & 0x8001)
            {
                FVector location = _playerMap->GetRandomLocation();
                //FVector location= {3000,3000};
                char type = RandomUtil::Rand(0,3);
                
                objectManager->SpawnMonster(location,type);
                inputDelay = 5000;
            }
        }
        
        auto toSpawn =   MAX_MONSTER - objectManager->Monsters();
        for(int i = 0 ; i<toSpawn;i++)
        {
            FVector location= {RandomUtil ::Rand(0,6300) + 50.0f,RandomUtil ::Rand(0,6300) + 50.0f};
            char type = RandomUtil ::Rand(0,1);
            objectManager->SpawnMonster(location,type);
        }

    }
    

    EasyMonsterGroup::~EasyMonsterGroup() = default;


    


}
