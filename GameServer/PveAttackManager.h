#pragma once
#include "../GameMap.h"
#include "../Base/AttackManager.h"

namespace psh
{
    class Monster;

    class PveAttackManager : public AttackManager
    {
    public:
        PveAttackManager(GameMap<shared_ptr<Monster>>& monster, GameMap<shared_ptr<Player>>& player):
                            _monsterMap(monster)
                          , _playerMap(player)
        {
        }

        bool GetClosestTarget(FVector location, weak_ptr<ChatCharacter>& target) override;
        void OnAttack(const AttackData& attack) override;
        GameMap<shared_ptr<Monster>>& _monsterMap;
        GameMap<shared_ptr<Player>>& _playerMap;
    };  
}

