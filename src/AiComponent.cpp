//
// Created by pshpj on 24. 10. 10.
//

#include "AiComponent.h"

#include <stdexcept>
#include <utility>

#include "AttackData.h"
#include "ChatCharacter.h"
#include "Field.h"


psh::MonsterAiComponent::MonsterAiComponent(ChatCharacter& owner, MonsterAi::TargetSelector selector)
    : _owner{&owner}
    , _selector{std::move(selector)}
    , _skills{ATTACK::GetSkillsByTemplate(owner.TemplateId())}
    , _spawnLocation{owner.Location()} {}

psh::MonsterAiComponent::~MonsterAiComponent() = default;

bool psh::MonsterAiComponent::IsReturning()
{
    if (_returning && !_owner->IsMoving())
    {
        _returning = false;
    }

    return _returning;
}

void psh::MonsterAiComponent::Update(int delta)
{
    if (_owner->isDead())
    {
        return;
    }

    _moveDelay.Update(delta);
    _searchDelay.Update(delta);

    if (IsReturning())
    {
        return;
    }

    const auto target = _target.lock();

    const auto location = _owner->Location();

    if (target == nullptr)
    {
        if (_searchDelay.IsExpired() == false)
        {
            return;
        }

        _target = _selector({location, _owner->ObjectId()}, _owner->GetField());
        _searchDelay.Reset(ACTION_DELAY);
        return;
    }

    //대상이 있는지 확인
    if (!target->Valid())
    {
        _target.reset();
        return;
    }

    auto targetLocation = target->Location();
    const auto tarDist = Distance(targetLocation, location);
    const auto curSkill = ATTACK::GetSkillInfoById(_skills[_curAttackIndex]);

    //공격 가능한 범위에 있다면
    if (tarDist < curSkill.skillSize.Y)
    {
        _owner->MoveStop();

        auto attackDir = (targetLocation - location).Normalize();
        attackDir = isnan(attackDir.X) ? _owner->ViewDirection() : attackDir;

        _owner->Attack(curSkill.id, attackDir);
        _curAttackIndex = (_curAttackIndex + 1) % _skills.size();
        _owner->ViewDirection(attackDir);
        return;
    }
    //범위 밖에 있는데 이동 명령 못하면 리턴.
    if (_moveDelay.IsExpired() == false)
    {
        return;
    }

    //범위 밖이고 이동도 가능하다.
    //너무 멀면 복귀
    if (Distance(targetLocation, _spawnLocation) >= MAX_MOVE_RADIUS)
    {
        _target.reset();
        _owner->MoveStart(_spawnLocation);
        _returning = true;
        return;
    }

    //아니면 추적
    _owner->MoveStart(targetLocation);
    _moveDelay.Reset(ACTION_DELAY);
}

