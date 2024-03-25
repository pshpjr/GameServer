#pragma once
#include "PveAttackManager.h"
#include "ObjectManager.h"

namespace psh
{
    class PveAttackManager;
}

namespace psh
{
    class Monster;
    class FieldObjectManager : public psh::ObjectManager
    {
    public:
        FieldObjectManager(GroupCommon& group,GameMap<shared_ptr<Player>>& player
            ,  GameMap<shared_ptr<Monster>>& monster, GameMap<shared_ptr<Item>>& item):
            ObjectManager(group,player)
            , _monsterMap(monster)
            , _itemMap(item)
        ,_attackManager(make_unique<PveAttackManager>(_monsterMap,_playerMap))
        {
        }

        FieldObjectManager(const FieldObjectManager& other) = delete;
        FieldObjectManager(FieldObjectManager&& other) = delete;
        FieldObjectManager& operator=(const FieldObjectManager& other) = delete;
        FieldObjectManager& operator=(FieldObjectManager&& other) = delete;
        

    protected:
        void GetAllGameMap(std::vector<GameMap<shared_ptr<GameObject>>*>& container) override;

    public:
        void InsertInGameMap(const shared_ptr<psh::GameObject>& actor, FVector location) override;
        void EraseToGameMap(const shared_ptr<psh::GameObject>& actor, FVector location) override;
        void OnActorMove(const shared_ptr<GameObject>& actor) override;
        void Update(int deltaMs) override;
        void CleanupActor(GameObject* actor) override;
        void SpawnItem(psh::FVector loc, char type);
        void SpawnMonster(FVector loc, char type);
        [[nodiscard]] int Monsters() const {return static_cast<int>(_monsters.size());}
    private:
        GameMap<shared_ptr<Monster>>& _monsterMap;
        HashMap<ObjectID, shared_ptr<Monster>> _monsters;
        
        GameMap<shared_ptr<Item>>& _itemMap;
        HashMap<ObjectID, shared_ptr<Item>> _items;

        unique_ptr<PveAttackManager> _attackManager;
    };
}
