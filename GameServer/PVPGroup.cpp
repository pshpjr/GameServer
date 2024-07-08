#include "PVPGroup.h"

#include "FieldObjectManager.h"
#include "PvpAttackManager.h"
#include "IOCP.h"
#include "Player.h"
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
        case eGame_ReqChat:
            RecvChat(id, recvBuffer);
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

void psh::PvpGroup::PvpResAttack(SessionID sessionId, CRecvBuffer& buffer)
{
    auto& [_, player] = *_players.find(sessionId);
    char type;
    psh::FVector dir;
    GetGame_ReqAttack(buffer, type, dir);

    if (player == nullptr)
    {
        printf("InvalidPlayer\n");
        _iocp->DisconnectSession(sessionId);
    }
    if (player->isDead())
    {
        printf("ResAttack playerDead, Account : %d\n", player->AccountNumber());
        return;
    }
    player->pvpAttackDebug(type, dir);
}

void psh::PvpGroup::PvpSendInRange(FVector location, std::span<const Sector> offsets, SendBuffer& buffer, const shared_ptr<psh::Player>& exclude)
{
    auto broadcastSectors = _playerMap->GetSectorsFromOffset(_playerMap->GetSector(location), offsets);



    ranges::for_each(broadcastSectors, [this, &buffer, &exclude](flat_unordered_set<shared_ptr<Player>>& sector)
        {
            for (auto& player : sector)
            {
                if (player == exclude)
                {
                    printf("continue\n");
                    continue;
                }
                printf("SendTo pvp Player : objid : %d\n", player->ObjectId());
                SendPacket(player->SessionId(), buffer);
            }
        });
}

psh::PvpGroup::~PvpGroup() = default;