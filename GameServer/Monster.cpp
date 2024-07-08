#include "Monster.h"

#include "AttackManager.h"
#include "ObjectManager.h"
#include "Profiler.h"
#include "GameMap.h"
#include "AttackData.h"

namespace psh
{
    constexpr int PLAYER_MOVE_SPEED = 200;
    constexpr int MAX_MOVE_RANGE = 800;
    constexpr int SEARCH_DELAY_MS = 2000;
    constexpr int ATTACK_DELAY_MS = 1000;


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
            _attackManager->GetClosestTarget(_spawnLocation, _target, MAX_MOVE_RANGE/2);
            searchCooldown += SEARCH_DELAY_MS;
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
        
            attackCooldown += ATTACK_DELAY_MS;
            return;
        }
        
        if((Location() - _spawnLocation).Size() > MAX_MOVE_RANGE)
        {
            MoveStart(_spawnLocation);
            _target.reset();
            moveCooldown += MAX_MOVE_RANGE/ PLAYER_MOVE_SPEED * 1000;
            return;
        }

        //몬스터가 과도하게 플레이어 잘 쫓아오는 것 막기 위해
        //1초 이동 가능한 범위에서 움직인다. 
        if(dist > PLAYER_MOVE_SPEED)
        {
            auto dest = Location() + (target->Location() - Location()).Normalize() * PLAYER_MOVE_SPEED;
            MoveStart(dest); 
        }
        else
        {
            MoveStart(target->Location());
        }
        
        moveCooldown += 1000;
        
    }
    
}
