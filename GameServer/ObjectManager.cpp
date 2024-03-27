#include "ObjectManager.h"

#include "GroupCommon.h"
#include "Player.h"
#include "TableData.h"


void psh::ObjectManager::RequestMove(const shared_ptr<psh::GameObject>& actor, psh::FVector nextLocation)
{
    _playerMap.ClamToMap(nextLocation);
    
    actor->OldLocation(actor->Location());
    actor->Location(nextLocation);
    OnActorMove(actor);
    const auto oldSector = _playerMap.GetSector(actor->OldLocation());
    const auto newSector = _playerMap.GetSector(nextLocation);
    // 같다면 리턴한다.
    if(oldSector == newSector)
    {
        return;
    }
    const auto sectorDiff = Clamp(newSector - oldSector,-1,1);
    const auto sectorIndex = TableIndexFromDiff(sectorDiff);

    const bool isPlayer = actor->ObjectGroup() == eCharacterGroup::Player ;
    
    RemoveFromMap(actor,actor->OldLocation(),SEND_OFFSETS::DeleteTable[sectorIndex.x][sectorIndex.y],false,isPlayer, ObjectManager::removeResult::Move);
    InsertInMap(actor,actor->Location(),SEND_OFFSETS::CreateTable[sectorIndex.x][sectorIndex.y],false, isPlayer);
}

void psh::ObjectManager::GetAllGameMap(std::vector<GameMap<shared_ptr<GameObject>>*>& container)
{
    container.push_back(reinterpret_cast<GameMap<shared_ptr<GameObject>>*>(&_playerMap));
}

void psh::ObjectManager::InsertInGameMap(const shared_ptr<psh::GameObject>& actor, psh::FVector location)
{
    _playerMap.Insert(static_pointer_cast<Player>(actor),location);
}

void psh::ObjectManager::EraseToGameMap(const shared_ptr<psh::GameObject>& actor, psh::FVector location)
{
    _playerMap.Delete(static_pointer_cast<Player>(actor),location);
}

void psh::ObjectManager::InsertInMap(const shared_ptr<psh::GameObject>& actor
                                     , const psh::FVector location
                                     , const std::span<const Sector> offsets
                                     , const bool isSpawn
                                     , const bool notifySelf
    )
{
    auto createThis = SendBuffer::Alloc();
    actor->MakeCreatePacket(createThis, isSpawn);
    _owner.SendInRange(location, offsets, createThis);

    actor->InMap(true);
    actor->SetNeedUpdate(true);
    if (!notifySelf)
    {
        InsertInGameMap(actor, location);
        return;
    }
    



    std::vector<GameMap<shared_ptr<GameObject>>*> gameMaps;
    GetAllGameMap(gameMaps);
    
    auto toCreate = SendBuffer::Alloc();
    auto sessionId = static_pointer_cast<const psh::Player>(actor)->SessionId();
    

    for (auto map : gameMaps)
    {
        auto nearbySectors = map->GetSectorsFromOffset(map->GetSector(location), offsets);
        ranges::for_each(nearbySectors, [this,&toCreate,sessionId](GameMap<shared_ptr<GameObject>>::container& sector)
        {
            for (auto& actor : sector)
            {
                actor->MakeCreatePacket(toCreate,false);
                if(toCreate.CanPushSize() < 111)
                {
                    _owner.SendPacket(sessionId,toCreate);
                    toCreate = SendBuffer::Alloc();
                }
            }
        });
    }
    if(toCreate.Size()!=0)
    {
        _owner.SendPacket(sessionId,toCreate);
    }


    InsertInGameMap(actor, location);
}

psh::ObjectManager::~ObjectManager() = default;

void psh::ObjectManager::SpawnActor(const shared_ptr<psh::GameObject>& actor,AttackManager* attackManager)
{
    bool isPlayer = actor->ObjectGroup() == eCharacterGroup::Player;

    if(isPlayer)
    {
        auto createThis = SendBuffer::Alloc();
        actor->MakeCreatePacket(createThis,true);
        _owner.SendPacket(static_pointer_cast<const psh::Player>(actor)->SessionId(),createThis);
    }

    InsertInMap(actor, actor->Location(), SEND_OFFSETS::BROADCAST, true, isPlayer);
    actor->_attackManager = attackManager;
    actor->OnCreate();
}

void psh::ObjectManager::DestroyActor(psh::GameObject* actor)
{
    _deleteWait.push_back(actor);
}

void psh::ObjectManager::RemoveFromMap(const shared_ptr<psh::GameObject>& actor
                                      , psh::FVector location
                                      , const std::span<const psh::Sector> offsets
                                      , bool isDead
                                      , bool notifySelf
                                      , char cause)
{
    //나는 맵에서 제거한 후에 나 삭제하라고 알린다.
    
    EraseToGameMap(actor, location);
    actor->InMap(false);
    actor->SetNeedUpdate(false);
    
    auto deleteThis = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(deleteThis,actor->ObjectId(),isDead,cause);
    _owner.SendInRange(location, offsets, deleteThis);

    if (!notifySelf)
    {
        return;
    }
    
    std::vector<GameMap<shared_ptr<GameObject>>*> gameMaps;
    GetAllGameMap(gameMaps);
    
    auto toDestroy = SendBuffer::Alloc();
    auto sessionId = static_pointer_cast<psh::Player>(actor)->SessionId();
    
    for (auto map : gameMaps)
    {
        auto nearbySectors = map->GetSectorsFromOffset(map->GetSector(location), offsets);
        ranges::for_each(nearbySectors, [this,&toDestroy,sessionId,cause](GameMap<shared_ptr<GameObject>>::container& sector)
        {
            for (auto& actor : sector)
            {
//                if (actor->ObjectGroup() == psh::eCharacterGroup::Player)
//{
//    if (static_pointer_cast<Player>(actor)->AccountNumber() == 666)
//    {
//        __debugbreak();
//    }
//
//
//}

                MakeGame_ResDestroyActor(toDestroy,actor->ObjectId(),false,cause);
                if(toDestroy.CanPushSize() < 111)
                {
                    _owner.SendPacket(sessionId,toDestroy);
                    toDestroy = SendBuffer::Alloc();
                }
            }
        });
    }
    
    if(toDestroy.Size()!=0)
    {
        _owner.SendPacket(sessionId,toDestroy);
    }
}
