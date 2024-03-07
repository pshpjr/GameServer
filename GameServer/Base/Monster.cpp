#include "Monster.h"
#include "../GameMap.h"
#include "../Data/AttackData.h"
#include "../Data/TableData.h"

namespace psh 
{
    constexpr int PLAYER_MOVE_SPEED = 400;
	
    Monster::Monster(ObjectID clientId, const psh::FVector& location, const psh::FVector& direction, char type)
    : ChatCharacter(clientId,location,direction,PLAYER_MOVE_SPEED,eCharacterGroup::Monster,type)
    {
        _attacks = monsterAttack[type];
    }
    

    void Monster::OnUpdate(float delta)
    {
        if(attackCooldown > 0)
            attackCooldown -=delta;
        if(moveCooldown > 0)
            moveCooldown -=delta;
        if(_target == nullptr)
        {
            _owner->GetClosestTarget(Location(),_target);
            return;
        }
        
        if(_target->isDead())
        {
            _target.reset();
            return;
        }
        auto dist = Distance(_target->Location(),Location());
        if((dist <= _attacks[0].first.Y))
        {
            if(attackCooldown >0)
                return;
            if(isMove())
                MoveStop();

            Attack(0);

            attackCooldown+=1000;
            return;
        }
 
        if(moveCooldown <=0)
        {
            MoveStart(_target->Location());
            moveCooldown+=1000;
        }
    }

    void Monster::Die()
    {
        ChatCharacter::Die();
    }

    void Monster::OnMove()
    {
        ChatCharacter::OnMove();
    }

    void Monster::MoveStart(FVector destination)
    {
        ChatCharacter::MoveStart(destination);

    }
}
