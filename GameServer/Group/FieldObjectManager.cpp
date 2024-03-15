#include "FieldObjectManager.h"
#include "../base/Player.h"
#include "../base/Monster.h"
#include "../Data/TableData.h"

void psh::FieldObjectManager::GetAllGameMap(std::vector<psh::GameMap<shared_ptr<psh::GameObject>>*>& container)
{
    container.push_back(reinterpret_cast<GameMap<shared_ptr<GameObject>>*>(&_playerMap));
    container.push_back(reinterpret_cast<GameMap<shared_ptr<GameObject>>*>(&_monsterMap));
    container.push_back(reinterpret_cast<GameMap<shared_ptr<GameObject>>*>(&_itemMap));
}

void psh::FieldObjectManager::InsertInGameMap(const shared_ptr<psh::GameObject>& actor, psh::FVector location)
{
    switch (actor->ObjectGroup())
    {
        case eCharacterGroup::Player:
            _playerMap.Insert(static_pointer_cast<psh::Player>(actor),location);
            break;
        case eCharacterGroup::Monster:
            _monsterMap.Insert(static_pointer_cast<psh::Monster>(actor),location);
            break;
        case eCharacterGroup::Object:
            ASSERT_CRASH(false,"Invalid Object");
            break;
        case eCharacterGroup::Item:
            _itemMap.Insert(static_pointer_cast<psh::Item>(actor),location);
            break;
    }
}

void psh::FieldObjectManager::EraseToGameMap(const shared_ptr<psh::GameObject>& actor, psh::FVector location)
{
    switch (actor->ObjectGroup())
    {
        case eCharacterGroup::Player:
            _playerMap.Delete(static_pointer_cast<psh::Player>(actor),location);
        break;
        case eCharacterGroup::Monster:
            _monsterMap.Delete(static_pointer_cast<psh::Monster>(actor),location);
        break;
        case eCharacterGroup::Object:
            ASSERT_CRASH(false,"Invalid Object");
        break;
        case eCharacterGroup::Item:
            _itemMap.Delete(static_pointer_cast<psh::Item>(actor),location);
        break;
    }
}

void psh::FieldObjectManager::OnActorMove(const shared_ptr<psh::GameObject>& actor)
{
    if(actor->ObjectGroup()!= eCharacterGroup::Player)
        return;
    
    auto items = _itemMap.GetSectorsFromOffset(_itemMap.GetSector(actor->Location()), SEND_OFFSETS::Single);
    ranges::for_each(items, [ this,&actor](flat_unordered_set<shared_ptr<Item>> sector)
    {
        for (auto& item : sector)
        {
            if (item->Collision(actor->Location()))
            {
                static_pointer_cast<Player>(actor)->GetCoin(1);

                item->Take(*static_pointer_cast<ChatCharacter>(actor));
            }
        }
    });
}

void psh::FieldObjectManager::Update(int deltaMs)
{
    if(inputDelay > 0)
        inputDelay-=deltaMs;
    else
    {
        if(GetAsyncKeyState('P') & 0x8001)
        {
            FVector location= {RandomUtil::Rand(0,6300) + 50.0f,RandomUtil::Rand(0,6300) + 50.0f};
            char type = RandomUtil::Rand(0,3);
            SpawnMonster(location,type);
            inputDelay = 5000;
        }

        if(GetAsyncKeyState('I') & 0x8001)
        {
            FVector location= {500,500};
            SpawnItem(location,0);
            inputDelay = 5000;
        }
    }
    
    for (auto& [_,actor] : _monsters)
    {
        actor->Update(deltaMs);
    }
}

void psh::FieldObjectManager::OnActorDestroy(GameObject& actor)
{
    ObjectManager::OnActorDestroy(actor);       
    if(actor.ObjectGroup() == eCharacterGroup::Monster)
    {
        SpawnItem(actor.Location(),0);
        _monsters.erase(actor.ObjectId());
    }
    else if(actor.ObjectGroup() == eCharacterGroup::Item)
    {
        SpawnItem(actor.Location(),0);
        _items.erase(actor.ObjectId());
    }
}

void psh::FieldObjectManager::SpawnItem(psh::FVector loc, char type)
{
    auto id = NextObjectId();
    printf("spawnItem %d\n", id);
        
    auto [item,success] = _items.emplace(id, make_shared<Item>(id,*static_cast<ObjectManager*>(this),_owner, loc,10, type));
    if (!success)
    {
        return;
    }
    SpawnActor(item->second,_attackManager.get());
}

void psh::FieldObjectManager::SpawnMonster(psh::FVector loc, char type)
{
    auto id = NextObjectId();
    printf("spawnActor %d\n", id);
        
    auto [monster,success] = _monsters.emplace(id, make_shared<Monster>(id,*static_cast<ObjectManager*>(this),_owner, loc, type));
    if (!success)
    {
        return;
    }
    monster->second->SetAttackManager(_attackManager.get());
    SpawnActor(monster->second,_attackManager.get());
}
