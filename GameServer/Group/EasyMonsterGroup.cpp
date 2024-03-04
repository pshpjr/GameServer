#include "EasyMonsterGroup.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "../Base/Player.h"
#include "../GameMap.h"
#include "../Server.h"
#include "Profiler.h"
#include "../Base/Item.h"
#include "../Base/Monster.h"
#include "../Base/Rand.h"
#include "../Data/TableData.h"
#include "../Base/Rand.h"
namespace psh
{
    EasyMonsterGroup::EasyMonsterGroup(Server* server,short mapSize, short sectorSize): GroupCommon(server, ServerType::Easy,mapSize,sectorSize)
    ,_monsterMap(make_unique<GameMap<ChatCharacter>>(mapSize,mapSize,server))
    ,_itemMap(make_unique<GameMap<Item>>(mapSize,mapSize,server)) {}

    void EasyMonsterGroup::OnEnter(SessionID id)
    {
        auto playerPtr = _server->getPlayerPtr(id);
        _players.emplace(id,playerPtr);
        _iocp->SetTimeout(id, 30000);

        playerPtr->ObjectId(GetNextID());
        playerPtr->SetGroup(this);
        playerPtr->SetMap(reinterpret_cast<GameMap<GameObject>*>(_playerMap.get()));
        
        auto levelInfoPacket = SendBuffer::Alloc();
        MakeGame_ResLevelEnter(levelInfoPacket,playerPtr->AccountNumber(),playerPtr->ObjectId(), _groupType);
        SendPacket(playerPtr->SessionId(), levelInfoPacket);
    }

    void EasyMonsterGroup::OnLeave(const SessionID id)
    {
        auto it = _players.find(id);
        auto& [_,playerPtr] = *it;

        SendDelete(playerPtr,playerPtr->Location(),SEND_OFFSETS::BROADCAST,false);

        _playerMap->Delete(playerPtr,playerPtr->Location());
        _players.erase(it);
    }



    void EasyMonsterGroup::OnRecv(const SessionID id, CRecvBuffer& recvBuffer)
    {
        ePacketType type;
        recvBuffer >> type;
        switch (type)
        {
        case eGame_ReqChangeComplete:
            OnChangeComp(id,recvBuffer);
            break;
        case eGame_ReqMove:
            OnMove(id,recvBuffer);
            break;
        case eGame_ReqAttack:
            OnAttack(id,recvBuffer);
            break;
        case eGame_ReqLevelChange:
            OnReqLevelChange(id,recvBuffer);
            break;
        default:
            DebugBreak();
            break;
        }
    }

    bool EasyMonsterGroup::SpawnItem(const shared_ptr<GameObject>& object)
    {
        auto id = GetNextID();
        //auto location= _monsterMap->GetRandomLocation();

        auto [item, success] = _items.emplace(id,make_shared<Item>(id,object->Location(),30.0f,0));
            
        if(!success)
        {
            return false;
        }
        item->second->SetGroup(this);
        item->second->SetMap(reinterpret_cast<GameMap<GameObject>*>(_itemMap.get()));
            
        _itemMap->Insert(item->second,object->Location());
        SendCreate(item->second,object->Location(),SEND_OFFSETS::BROADCAST,true);
        return true;
    }

    void EasyMonsterGroup::OnActorDestroy(const shared_ptr<GameObject>& object)
    {
        switch (object->ObjectGroup())
        {
        case eCharacterGroup::Player:
            {
                _iocp->DisconnectSession(static_pointer_cast<Player>(object)->SessionId());
            }
            break;
        case eCharacterGroup::Monster:
            {
                if (!SpawnItem(object))
                {
                    return;
                }
                
                _monsterMap->Delete(static_pointer_cast<Monster>(object) ,object->Location());
                _monsters.erase(object->ObjectId());
            }
            break;
        case eCharacterGroup::Object:
            break;
        case eCharacterGroup::Item:
            {
                _itemMap->Delete(static_pointer_cast<Item>(object) ,object->Location());
                _monsters.erase(object->ObjectId());
            }
            break;
        default: ;
        }
    }

    void EasyMonsterGroup::GetActors(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets)
    {
        //정보를 받아올 패킷을 만들고
        vector<SendBuffer> toCreate;
        toCreate.reserve(4);
        toCreate.push_back(SendBuffer::Alloc());
        
        //몬스터 맵에서 순회하며 정보를 받아온다.
        auto oldSector = _monsterMap->GetSector(location);
        auto monsterSector = _monsterMap->GetSectorsFromOffset(oldSector,offsets);
        ranges::for_each(monsterSector,[this,&toCreate](flat_unordered_set<shared_ptr<ChatCharacter>> sector){
             for(auto& player : sector)
             {
                  if (toCreate.back().CanPushSize() < 111)
                  {
                      toCreate.push_back(SendBuffer::Alloc());
                  }
                  player->GetInfo(toCreate.back(),false);
             }
        });
        
        //아이템 맵에서 순회하며 정보를 받아온다.
        auto itemSector = _itemMap->GetSectorsFromOffset(oldSector,offsets);
        ranges::for_each(itemSector,[this,&toCreate](flat_unordered_set<shared_ptr<Item>> sector){
             for(auto& item : sector)
             {
                  if (toCreate.back().CanPushSize() < 111)
                  {
                      toCreate.push_back(SendBuffer::Alloc());
                  }
                  item->GetInfo(toCreate.back(),false);
             }
        });
        //있으면 전송한다. 

        if(toCreate.back().Size() !=0){
            SendPackets(static_pointer_cast<Player>(object)->SessionId(),toCreate);
        }
    }

    void EasyMonsterGroup::OnChangeComp(const SessionID id, CRecvBuffer& recvBuffer)
    {
        AccountNo accountNo;
        GetGame_ReqChangeComplete(recvBuffer,accountNo);
        
        auto& [_,playerPtr] = *_players.find(id);
        auto createBuffer = SendBuffer::Alloc();
        playerPtr->GetInfo(createBuffer,true);
        SendPacket(playerPtr->SessionId(),createBuffer);
        
        SendCreateAndGetInfo(playerPtr,playerPtr->Location(),SEND_OFFSETS::BROADCAST,false);
        GetActors(playerPtr,playerPtr->Location(),SEND_OFFSETS::BROADCAST);
        _playerMap->Insert(playerPtr,playerPtr->Location());
    }

    void EasyMonsterGroup::OnReqLevelChange(const SessionID id, CRecvBuffer& recvBuffer) const
    {
        AccountNo accountNo;
        ServerType type;
        GetGame_ReqLevelChange(recvBuffer,accountNo,type);
        
        MoveSession(id,_server->GetGroupID(type));
    }

    void EasyMonsterGroup::UpdateContent(const float delta)
    {
        if(GetAsyncKeyState('P') & 8001)
        {
            SpawnMonster();
        }
        
        for(auto& [_,actor] : _players)
        {
            actor->Update(delta);
        }

        for(auto& [_,actor] : _monsters)
        {
            actor->Update(delta);
        }
    }

    void EasyMonsterGroup::SendMonitor()
    {
    }

    void EasyMonsterGroup::CheckVictim(const Range& attackRange, int damage, const shared_ptr<ChatCharacter>& attacker)
    {
        auto victims = attacker->ObjectGroup() == eCharacterGroup::Player ?
            _monsterMap->GetSectorsFromRange(attackRange)
                :_playerMap->GetSectorsFromRange(attackRange);
        
        
        ranges::for_each(victims,[ this,damage,&attacker,&attackRange](flat_unordered_set<shared_ptr<ChatCharacter>> sector)
        {
            for(auto& player : sector)
            {
                if(player == attacker)
                    continue;
                        
                if(attackRange.Contains(player->Location()))
                {
                    player->Hit(damage,attacker);
                }
            }
        });
    }

    void EasyMonsterGroup::CheckItem(const shared_ptr<ChatCharacter>& target)
    {
        
        auto items = _itemMap->GetSectorsFromOffset(_itemMap->GetSector(target->Location()),SEND_OFFSETS::Single);
        ranges::for_each(items,[ this,&target](flat_unordered_set<shared_ptr<Item>> sector)
        {
            for(auto& item : sector)
            {
                if(item->Collision(target->Location()))
                {
                    static_pointer_cast<Player>(target)->GetCoin();
                    item->Destroy(false);
                }
            }
        });
    }

    EasyMonsterGroup::~EasyMonsterGroup() = default;

    void EasyMonsterGroup::OnAttack(const SessionID sessionId, CRecvBuffer& buffer)
    {
        auto& [_,player] = *_players.find(sessionId);
        char type;
        GetGame_ReqAttack(buffer,type);
		
        if(player == nullptr)
        {
            _iocp->DisconnectSession(sessionId);
        }

        player->Attack(type);
    }

    
    void EasyMonsterGroup::SpawnMonster()
    {
        auto id = GetNextID();
        //auto location= _monsterMap->GetRandomLocation();
        FVector location= {RandomUtil::Rand(800,1000) + 100.0f,RandomUtil::Rand(800,1000) + 100.0f};
        FVector dir= {0,0};
        auto [monster,success] = _monsters.emplace(id,make_shared<Monster>(id,location,dir,RandomUtil::Rand(0,3)));
        if(!success)
        {
            return;
        }
        monster->second->SetGroup(this);
        monster->second->SetMap(reinterpret_cast<GameMap<GameObject>*>(_monsterMap.get()));
        _monsterMap->Insert(monster->second,location);
        SendCreate(monster->second,location,SEND_OFFSETS::BROADCAST,true);
    }

    void EasyMonsterGroup::OnMove(const SessionID sessionId, CRecvBuffer& buffer)
    {
        //printf(format("Request isMove {:d} \n",sessionId.id).c_str());
        FVector location;
        GetGame_ReqMove(buffer,location);
        
        auto result = _players[sessionId];
        if(result == nullptr)
        {
            _iocp->DisconnectSession(sessionId);
        }

        location = Clamp(location,0,_playerMap->Size());
        
        result->MoveStart(location);
    }
}
