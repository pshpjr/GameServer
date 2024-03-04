#include "GroupCommon.h"

#include "../GameMap.h"
#include "../Sector.h"
#include "../Data/TableData.h"
#include "../Base/ChatCharacter.h"
#include "../Base/Player.h"


void psh::GroupCommon::OnUpdate()
{
    int delta = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - _prevUpdate).count();

    _prevUpdate += chrono::milliseconds(delta);
    UpdateContent(10);
    _fps++;

    
    if ( std::chrono::steady_clock::now() < _nextDBSend )
    {
        return ;
    }


    if(!_useDB)
    {
    }

    SendMonitor();



    _nextDBSend += 1s;
    _fps = 0;
}

psh::GroupCommon::~GroupCommon()
{
}

void psh::GroupCommon::Broadcast(FVector location, SendBuffer& buffer, GameObject* exclude)
{
    auto broadcastSectors = _playerMap->GetSectorsFromOffset(_playerMap->GetSector(location),SEND_OFFSETS::BROADCAST);
    ranges::for_each(broadcastSectors,[ this,&buffer,&exclude](flat_unordered_set<shared_ptr<ChatCharacter>> sector)
    {
        for(auto& player : sector)
        {
            if(player.get() == exclude)
                continue;
            
            SendPacket(static_pointer_cast<Player>(player)->SessionId(), buffer);
        }
    });
}

psh::Sector TableIndexFromDiff(const psh::Sector sectorDiff)
{
    return psh::Sector(sectorDiff.x + 1,sectorDiff.y+1);
}

void psh::GroupCommon::BroadcastMove(const shared_ptr<ChatCharacter>& player, FVector oldLocation, FVector newLocation)
{
    auto& map = *player->Map();
    //대상이 있는 섹터와 다음 섹터를 구한다.
    const auto oldSector = map.GetSector(oldLocation);
    const auto newSector = map.GetSector(newLocation);
    // 같다면 리턴한다.
    if(oldSector == newSector)
    {
        return;
    }
    // 다르다면 전송해야 할 범위를 구한다.
    const auto sectorDiff = Clamp(newSector - oldSector,-1,1);
    const auto sectorIndex = TableIndexFromDiff(sectorDiff);
    
    //맵에서 삭제한다. 
    map.Delete(player,oldLocation);
    SendDeleteAndGetInfo(player,oldLocation,SEND_OFFSETS::DeleteTable[sectorIndex.x][sectorIndex.y],false);
    
    SendCreateAndGetInfo(player,newLocation,SEND_OFFSETS::CreateTable[sectorIndex.x][sectorIndex.y],false);
    map.Insert(player,newLocation);
}

void psh::GroupCommon::SendCreate(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets, bool isSpawn)
{
    SendBuffer createThis = SendBuffer::Alloc();
    object->GetInfo(createThis,isSpawn);

    auto oldSector = _playerMap->GetSector(location);
    
    // 주변 모두에게 전송
    auto addSectors = _playerMap->GetSectorsFromOffset(oldSector,offsets);
    
    ranges::for_each(addSectors,[this,&createThis](flat_unordered_set<shared_ptr<ChatCharacter>> sector){
         for(auto& player : sector)
         {
             SendPacket(static_pointer_cast<Player>(player)->SessionId(),createThis);
         }
    });
}

void psh::GroupCommon::SendCreateAndGetInfo(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets, bool isSpawn)
{
    SendBuffer createThis = SendBuffer::Alloc();
    object->GetInfo(createThis,isSpawn);
    
    vector<SendBuffer> toCreate;
    toCreate.reserve(4);
    toCreate.push_back(SendBuffer::Alloc());

    auto oldSector = _playerMap->GetSector(location);
    
    // 주변 모두에게 전송하고, 그 플레이어들의 정보를 받아온다.(나는 없으니까 제외 안 해도 됨)
    auto addSectors = _playerMap->GetSectorsFromOffset(oldSector,offsets);

    //실행시킨다. 
    ranges::for_each(addSectors,[this,&createThis,&toCreate](flat_unordered_set<shared_ptr<ChatCharacter>> sector){
         for(auto& player : sector)
         {
              SendPacket(static_pointer_cast<Player>(player)->SessionId(),createThis);
              if (toCreate.back().CanPushSize() < 111)
              {
                  toCreate.push_back(SendBuffer::Alloc());
              }
              player->GetInfo(toCreate.back(),false);
         }
    });

    if(toCreate.back().Size() !=0){
        SendPackets(static_pointer_cast<Player>(object)->SessionId(),toCreate);
    }
}

void psh::GroupCommon::SendDelete(const shared_ptr<psh::GameObject>& object, FVector location,
                                  const std::span<const psh::Sector> offsets, bool isDead)
{
    SendBuffer deleteThis = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(deleteThis,object->ObjectId(),isDead);

    auto oldSector = _playerMap->GetSector(location);
    
    // 주변 모두에게 전송하고, 그 플레이어들의 정보를 받아온다.(나는 없으니까 제외 안 해도 됨)
    auto delSectors = _playerMap->GetSectorsFromOffset(oldSector,offsets);
    //실행시킨다. 
    ranges::for_each(delSectors,[this,&deleteThis](flat_unordered_set<shared_ptr<ChatCharacter>> sector){
         for(auto& player : sector)
         {
             SendPacket(static_pointer_cast<Player>(player)->SessionId(),deleteThis);
         }
    });
}

void psh::GroupCommon::SendDeleteAndGetInfo(const shared_ptr<psh::GameObject>& object, FVector location,
                                            const std::span<const psh::Sector> offsets, bool isDead)
{
    SendBuffer deleteThis = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(deleteThis,object->ObjectId(),isDead);
    
    vector<SendBuffer> toDelete;
    toDelete.reserve(4);
    toDelete.push_back(SendBuffer::Alloc());

    auto oldSector = _playerMap->GetSector(location);
    
    // 주변 모두에게 전송하고, 그 플레이어들의 정보를 받아온다.(나는 없으니까 제외 안 해도 됨)
    auto delSectors = _playerMap->GetSectorsFromOffset(oldSector,offsets);

    //실행시킨다. 
    ranges::for_each(delSectors,[this,&deleteThis,&toDelete,isDead](flat_unordered_set<shared_ptr<ChatCharacter>> sector){
         for(auto& player : sector)
         {
              SendPacket(static_pointer_cast<Player>(player)->SessionId(),deleteThis);
              if (toDelete.back().CanPushSize() < 111)
              {
                  toDelete.push_back(SendBuffer::Alloc());
              }
              MakeGame_ResDestroyActor(toDelete.back(),player->ObjectId(),isDead);
         }
    });

    if(toDelete.back().Size() !=0){
        SendPackets(static_pointer_cast<Player>(object)->SessionId(),toDelete);
    }
}

