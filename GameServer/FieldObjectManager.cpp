#include "FieldObjectManager.h"

#include "Profiler.h"
#include "Player.h"
#include "Monster.h"
#include "TableData.h"

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
    ranges::for_each(items, [ this,&actor](GameMap<shared_ptr<Item>>::container& sector)
    {
        for (auto& item : sector)
        {
            if (item->Collision(actor->Location())&& item->isValid())
            {
                static_pointer_cast<Player>(actor)->GetCoin(1);
                

                item->Take(*static_pointer_cast<ChatCharacter>(actor));
            }
        }
    });
}

void psh::FieldObjectManager::Update(int deltaMs)
{
    PRO_BEGIN(L"FieldManagerUpdateMonster");
    for (auto& [_,actor] : _monsters)
    {
        if(actor->NeedUpdate())
            actor->Update(deltaMs);
    }
}

void psh::FieldObjectManager::CleanupActor(GameObject* actor)
{
    if(actor->ObjectGroup() == eCharacterGroup::Monster)
    {
        SpawnItem(actor->Location(),0);
        _monsters.erase(actor->ObjectId());
    }
    else if(actor->ObjectGroup() == eCharacterGroup::Item)
    {
        _items.erase(actor->ObjectId());
    }
    //플레이어면 아무것도 안 함. 
}


void psh::FieldObjectManager::SpawnItem(psh::FVector loc, char type)
{
    auto id = NextObjectId();

    auto [item,success] = _items.emplace(id, make_shared<Item>(id,*static_cast<ObjectManager*>(this),_owner, loc,static_cast<float>( 20), type));
    if (!success)
    {
        return;
    }
    SpawnActor(item->second,_attackManager.get());
}

void psh::FieldObjectManager::SpawnMonster(psh::FVector loc, char type)
{
    auto id = NextObjectId();

    auto [monster,success] = _monsters.emplace(id, make_shared<Monster>(id,*static_cast<ObjectManager*>(this),_owner, loc, type));
    if (!success)
    {
        return;
    }
    monster->second->SetAttackManager(_attackManager.get());
    SpawnActor(monster->second,_attackManager.get());
}
