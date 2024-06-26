#include "PVPGroup.h"

#include "FieldObjectManager.h"
#include "PvpAttackManager.h"

psh::PvpGroup::PvpGroup(Server& server, const ServerInitData& data, ServerType type, short mapSize, short sectorSize)
: 
                                                            GroupCommon(server
                                                                        ,data
                                                                        , type
                                                                        ,mapSize, sectorSize)
                                                            , _monsterMap(make_unique<GameMap<shared_ptr<Monster>>>(mapSize, sectorSize))
                                                            , _itemMap(make_unique<GameMap<shared_ptr<Item>>>(mapSize, sectorSize))
{
    _attackManager = static_cast<unique_ptr<AttackManager>>(make_unique<PvpAttackManager>(*_monsterMap, *_playerMap));
    _objectManager = 
    unique_ptr<ObjectManager>{
        static_cast<psh::ObjectManager*>(
            new FieldObjectManager(*this,*_playerMap,*_monsterMap,*_itemMap,_attackManager.get()))};

}

void psh::PvpGroup::OnRecv(SessionID id, CRecvBuffer& recvBuffer)
{
    ePacketType type;
    recvBuffer >> type;
    switch (type)
    {
        case eGame_ReqChangeComplete:
            RecvChangeComp(id, recvBuffer);
            debugData.revChange++;
        break;
        case eGame_ReqMove:
            RecvMove(id, recvBuffer);
            debugData.move++;
        break;
        case eGame_ReqAttack:
            RecvAttack(id, recvBuffer);
            debugData.attack++;
        break;
        case eGame_ReqLevelEnter:
            RecvReqLevelChange(id, recvBuffer);
            debugData.reqLevelChange++;
        break;
        default:
            DebugBreak();
        break;
    }
}

void psh::PvpGroup::UpdateContent(int deltaMs)
{
    GroupCommon::UpdateContent(deltaMs);
    auto objectManager = static_cast<FieldObjectManager*>(_objectManager.get());


    if(inputDelay > 0)
        inputDelay-=deltaMs;
    else
    {
        if(GetAsyncKeyState('P') & 0x8001)
        {
            //FVector location = _playerMap->GetRandomLocation();
            FVector location= {3000,3000};
            char type = RandomUtil::Rand(0,3);
                
            objectManager->SpawnMonster(location,type);
            inputDelay = 5000;
        }
    }
        
    auto toSpawn =   MAX_MONSTER - objectManager->Monsters();
    for(int i = 0 ; i<toSpawn;i++)
    {
        FVector location= {RandomUtil ::Rand(0,6300) + 50.0f,RandomUtil ::Rand(0,6300) + 50.0f};
        char type = RandomUtil ::Rand(2,3);
        objectManager->SpawnMonster(location,type);
    }
}

void psh::PvpGroup::SendMonitor()
{
    GroupCommon::SendMonitor();

    debugData.revChange = 0;
    debugData.move = 0;
    debugData.attack = 0;
    debugData.reqLevelChange = 0;

}

psh::PvpGroup::~PvpGroup() = default;