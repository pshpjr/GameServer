#include "Monster.h"

#include "AttackManager.h"
#include "ObjectManager.h"
#include "Profiler.h"
#include "GameMap.h"
#include "AttackData.h"

namespace psh
{
    constexpr int PLAYER_MOVE_SPEED = 200;

    Monster::Monster(ObjectID clientId,ObjectManager& manager, GroupCommon& group, const FVector& location, char type)
        : ChatCharacter(clientId, manager,group,location,  PLAYER_MOVE_SPEED, eCharacterGroup::Monster, type)
        ,_spawnLocation(location)
    {
        _attacks = ATTACK::monsterAttack[type];
    }


    void Monster::OnUpdate(int delta)
    {
        PRO_BEGIN("MonsterOnUpdate")
        if (attackCooldown > 0)
        {
            attackCooldown -= delta;
        }
        if (searchCooldown > 0)
        {
            searchCooldown -= delta;
        }
        if (moveCooldown > 0)
        {
            moveCooldown -= delta;
            return;
        }
        
        auto target = _target.lock();

        if (target == nullptr)
        {
            if (searchCooldown > 0)
                return;

            PRO_BEGIN("GetClosestTarget")
            _attackManager->GetClosestTarget(_spawnLocation, _target);
            searchCooldown += 2000;
            searchCooldown += 2000;
            auto newTarget = _target.lock();
            return;
        }
        
        if (target->isDead())
        {
            _target.reset();
            return;
        }
        auto dist = Distance(target->Location(), Location());
        if ((dist < _attacks[0].first.Y))
        {
            if (attackCooldown > 0)
            {
                return;
            }
            if (isMove())
            {
                MoveStop();
            }
            

            auto attackDir = (target->Location() - Location()).Normalize();

            attackDir = isnan(attackDir.X) ? Direction() : attackDir;
 
            Attack(0, attackDir);
        
            attackCooldown += 1000;
            return;
        }
        
        if((Location() - _spawnLocation).Size() > 800)
        {
            MoveStart(_spawnLocation);
            target = nullptr;
            moveCooldown += 4000;
            return;
        }

        if(dist > 200)
        {
            auto dest = Location() + (target->Location() - Location()).Normalize() * 200;
            MoveStart(dest); 
        }
        else
        {
            MoveStart(target->Location());
        }
        
        moveCooldown += 1000;
        
    }
    
}
