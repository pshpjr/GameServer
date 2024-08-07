﻿#include "PvpAttackManager.h"
#include "ChatCharacter.h"
#include "Player.h"
#include "Monster.h"
#include "AttackData.h"
#include "TableData.h"

bool psh::PvpAttackManager::GetClosestTarget(FVector location, weak_ptr<ChatCharacter>& target , float maxDist)
{
    bool find = false;
    float closest = maxDist;
    
    auto players = _playerMap.GetSectorsFromOffset(_playerMap.GetSector(location),SEND_OFFSETS::BROADCAST);
    ranges::for_each(players,[ this,&target,&closest,&find,location](flat_unordered_set<shared_ptr<Player>> sector)
    {
        for(auto& player : sector)
        {
            auto dist = Distance(player->Location(),location);
            if( dist < closest)
            {
                closest = dist;
                target = player;
                find = true;
            }
        }
    });
    
    return find;
}

void psh::PvpAttackManager::OnAttack(const AttackData& attack)
{
    auto monsters = _monsterMap.GetSectorsFromRange(*attack.Range);
    ranges::for_each(monsters, [ this,&attack](
             flat_unordered_set<shared_ptr<Monster>> sector)
             {
                 for (auto& monster : sector)
                 {
                     if (monster->isDead())
                         continue;

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

    auto players = _playerMap.GetSectorsFromRange(*attack.Range);
    ranges::for_each(players, [ this,&attack](
             flat_unordered_set<shared_ptr<Player>> sector)
             {
                 for (auto& player : sector)
                 {
                     if (player->isDead())
                         continue;

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
