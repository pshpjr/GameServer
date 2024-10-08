#include "ChatCharacter.h"
#include "AttackData.h"
#include "Field.h"
#include "Player.h"
#include "RangeObject.h"
#include "TableData.h"

void psh::ChatCharacter::MakeCreatePacket(SendBuffer &buffer, const bool spawn) const
{
    GameObject::MakeCreatePacket(buffer, spawn);
    MakeGame_ResChracterDetail(buffer, ObjectId(), _hp);
}

void psh::ChatCharacter::Attack(const SkillID type, const FVector dir)
{
    auto &[_,skill] = *_skills.find(type);
    //만료 확인
    if (skill.timer.IsExpired() == false)
    {
        return;
    }

    ATTACK::ExecuteAttack({*this, dir, type});

    //쿨타임 추가.
    skill.timer.Reset(Timer::ms{0});
}

void psh::ChatCharacter::Hit(const DamageInfo info)
{
    //각종 데미지 계산 공식...
    _hp -= info.damage;

    auto hitPacket = SendBuffer::Alloc();
    MakeGame_ResHit(hitPacket, ObjectId(), info.attacker, _hp);

    for (auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
         auto &player: view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(hitPacket);
    }

    if (_hp <= 0)
    {
        Die();
    }
}


void psh::ChatCharacter::Die()
{
    DieImpl();
    _field.DestroyActor(shared_from_this());
}

