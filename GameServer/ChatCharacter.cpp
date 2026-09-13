#include "ChatCharacter.h"

#include "AiComponent.h"
#include "AttackData.h"
#include "Field.h"
#include "MoveComponent.h"
#include "TableData.h"


bool psh::ChatCharacter::IsCooldownEndDebug(SkillID type)
{
    auto it = _skills.find(type);
    if (it == _skills.end())
    {
        ASSERT_CRASH(false, "No skill found");
    }
    auto& [_,skill] = *it;

    //만료 확인
    return skill.timer.IsExpired();
}

psh::ChatCharacter::ChatCharacter(Field& group, const GameObjectData& initData
                                  , std::unique_ptr<MonsterAiComponent> aiComponent)
    : GameObject(group, initData)
      , _movementComponent{std::make_unique<MoveComponent>(*this, initData.moveSpeedPerSec / 1000.0f)}
      , _aiComponent{std::move(aiComponent)}
{
    for (auto skill : ATTACK::GetSkillsByTemplate(initData.templateId))
    {
        _skills.emplace(skill, SkillCooldown{ATTACK::GetSkillInfoById(skill), {}});
    }
}

psh::ChatCharacter::~ChatCharacter() = default;

void psh::ChatCharacter::OnUpdate(int delta)
{
    for (auto& skill : _skills)
    {
        skill.second.timer.Update(delta);
    }

//TODO: 이런 컴포넌트들이 객체 안에 있으면 안 될 것 같음.
    _movementComponent->Update(delta);
    if (_aiComponent)
    {
        _aiComponent->Update(delta);
    }
}

void psh::ChatCharacter::MoveStart(FVector destination) const
{
    _movementComponent->MoveStart(destination);
}

void psh::ChatCharacter::MoveStop() const
{
    _movementComponent->MoveStop();
}

bool psh::ChatCharacter::IsMoving() const
{
    return _movementComponent->IsMoving();
}

psh::MoveComponent& psh::ChatCharacter::GetMovable() const
{
    return *_movementComponent;
}

void psh::ChatCharacter::MakeCreatePacket(SendBuffer& buffer, const bool spawn) const
{
    GameObject::MakeCreatePacket(buffer, spawn);

    if (IsMoving())
    {
        MakeGame_ResMove(buffer, ObjectId(), ObjectType(), _movementComponent->Destination());
    }

    MakeGame_ResChracterDetail(buffer, ObjectId(), _hp);
}

void psh::ChatCharacter::SetAi(unique<MonsterAiComponent> component)
{
    _aiComponent = std::move(component);
}


void psh::ChatCharacter::Attack(const SkillID type, const FVector dir)
{
    auto it = _skills.find(type);
    if (it == _skills.end())
    {
        ASSERT_CRASH(false, "No skill found");
    }
    auto& [_,skill] = *it;
    //만료 확인
    if (skill.timer.IsExpired() == false)
    {
        return;
    }

    ATTACK::ExecuteAttack({*this, dir, type});

    //쿨타임 추가.
    skill.timer.Reset(skill.skillInfo.cooldown);
}

void psh::ChatCharacter::Hit(const DamageInfo info)
{
    if (Valid() == false)
    {
        return;
    }
    //각종 데미지 계산 공식...
    _hp -= info.damage;

    auto hitPacket = SendBuffer::Alloc();
    MakeGame_ResHit(hitPacket, ObjectId(), info.attacker, _hp);

    _field.BroadcastToPlayer(Location(), {hitPacket});

    if (_hp <= 0)
    {
        Die();
    }
}

bool psh::ChatCharacter::IsDead() const
{
    return _hp <= 0;
}

void psh::ChatCharacter::Revive()
{
    _hp = 100;
}


void psh::ChatCharacter::Die()
{
    RemoveReason(removeReason::Die);
    DieImpl();
    _field.DestroyActor(shared_from_this());
}

void psh::ChatCharacter::OnDestroyImpl()
{
}

int psh::ChatCharacter::Hp()
{
    return _hp;
}
