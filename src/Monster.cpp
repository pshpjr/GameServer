#include "Monster.h"

#include "AttackData.h"
#include "Field.h"
#include "GameMap.h"
#include "Player.h"
#include "Profiler.h"
#include "RangeObject.h"
#include "TableData.h"

namespace psh
{
    constexpr auto PLAYER_MOVE_SPEED = 200;
    constexpr auto MAX_MOVE_RANGE = 800;
    constexpr auto SEARCH_DELAY_MS = 2000;
    constexpr auto ATTACK_DELAY_MS = 1000;

    Monster::Monster(Field &group, const GameObjectData &initData)
        : ChatCharacter(group, initData)
      , attackRange{ATTACK::GetAIRangeByTemplate(initData.templateId)}
      , _spawnLocation{initData.location}
    {
    }


    void Monster::OnUpdate(const int delta)
    {
        PRO_BEGIN(MonsterOnUpdate)
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

        const auto target = _target.lock();

        if (target == nullptr)
        {
            if (searchCooldown > 0)
            {
                return;
            }

            PRO_BEGIN(GetClosestTarget);
            _target = _selector(Location(), &_field);
            //_attackStrategy->GetClosestTarget(_spawnLocation, _target, MAX_MOVE_RANGE/2);
            searchCooldown += SEARCH_DELAY_MS;
            return;
        }

        if (target->isDead())
        {
            _target.reset();
            return;
        }

        const auto dist = Distance(target->Location(), Location());
        if ((dist < _skills[0].skillInfo.skillSize.X))
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

        if ((Location() - _spawnLocation).Size() > MAX_MOVE_RANGE)
        {
            MoveStart(_spawnLocation);
            _target.reset();
            moveCooldown += MAX_MOVE_RANGE / PLAYER_MOVE_SPEED * 1000;
            return;
        }

        //몬스터가 과도하게 플레이어 잘 쫓아오는 것 막기 위해
        //1초 이동 가능한 범위에서 움직인다. 
        if (dist > PLAYER_MOVE_SPEED)
        {
            const auto dest = Location() + (target->Location() - Location()).Normalize() * PLAYER_MOVE_SPEED;
            MoveStart(dest);
        }
        else
        {
            MoveStart(target->Location());
        }

        moveCooldown += 1000;
    }

    void Monster::OnDestroy()
    {
        ChatCharacter::OnDestroy();

        auto destroyThis = SendBuffer::Alloc();
        MakeGame_ResDestroyActor(destroyThis, ObjectId(), isDead(), 0);
        for (auto &p: _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST))
        {
            std::static_pointer_cast<Player>(p)->SendPacket(destroyThis);
        }

        GameObjectData itemData{Location(), {0, 0}, 0, eObjectType::Item, 100};

        const auto obj = std::make_shared<Item>(_field, itemData, ATTACK::GetRangeByItemID(100));

        _field.SpawnItem(obj);
    }
}
