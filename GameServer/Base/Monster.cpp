#include "Monster.h"

#include "AttackManager.h"
#include "ObjectManager.h"
#include "../GameMap.h"
#include "../Data/AttackData.h"
#include "../Data/TableData.h"

namespace psh
{
    constexpr int PLAYER_MOVE_SPEED = 400;

    Monster::Monster(ObjectID clientId,ObjectManager& manager, GroupCommon& group, const FVector& location, char type)
        : ChatCharacter(clientId, manager,group,location,  PLAYER_MOVE_SPEED, eCharacterGroup::Monster, type)
    {
        _attacks = monsterAttack[type];
    }


    void Monster::OnUpdate(float delta)
    {
        if (attackCooldown > 0)
        {
            attackCooldown -= delta;
        }
        if (moveCooldown > 0)
        {
            moveCooldown -= delta;
        }
        
        auto target = _target.lock();
        
        if (target == nullptr)
        {
            _attackManager->GetClosestTarget(Location(), _target);
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
        
            Attack(0);
        
            attackCooldown += 1000;
            return;
        }
        
        if (moveCooldown <= 0)
        {
            MoveStart(target->Location());
            moveCooldown += 1000;
        }
    }
    
}
