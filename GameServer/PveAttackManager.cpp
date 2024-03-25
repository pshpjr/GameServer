#include "PveAttackManager.h"

#include "ChatCharacter.h"
#include "Player.h"
#include "Monster.h"
#include "AttackData.h"
#include "TableData.h"

bool psh::PveAttackManager::GetClosestTarget(FVector location, weak_ptr<ChatCharacter>& target)
{
     bool find = false;
    float closest = 99999;
    
    auto nearbySectors = _playerMap.GetSectorsFromOffset(_playerMap.GetSector(location),SEND_OFFSETS::BROADCAST);
    ranges::for_each(nearbySectors,[ this,&target,&closest,&find,location](flat_unordered_set<shared_ptr<Player>> sector)
    {
        for(auto& player : sector)
        {
            auto dist = Distance(player->Location(),location);
            if( dist < closest)
            {
                target = player;
                find = true;
            }
        }
    });
    return find;
}

void psh::PveAttackManager::OnAttack(const AttackData& attack)
{
    if(attack.AttackerType == eCharacterGroup::Player)
    {
        auto victims = _monsterMap.GetSectorsFromRange(*attack.Range);
        ranges::for_each(victims, [ this,&attack](
                 flat_unordered_set<shared_ptr<Monster>> sector)
                 {
                     for (auto& monster : sector)
                     {
                         if (monster->ObjectId() == attack.Attacker)
                         {
                             continue;
                         }

                         if (attack.Range->Contains(monster->Location()))
                         {
                             monster->Hit(attack.Damage, attack.Attacker);
                         }
                     }
                 });
    }
    else
    {
        auto victims = _playerMap.GetSectorsFromRange(*attack.Range);
        ranges::for_each(victims, [ this,&attack](
                 flat_unordered_set<shared_ptr<Player>> sector)
                 {
                     for (auto& player : sector)
                     {
                         if (player->ObjectId() == attack.Attacker)
                         {
                             continue;
                         }

                         if (attack.Range->Contains(player->Location()))
                         {
                             player->Hit(attack.Damage, attack.Attacker);
                         }
                     }
                 });
    }

    

}
