#include "ChatCharacter.h"
#include "AttackData.h"
#include "Field.h"
#include "Player.h"
#include "TableData.h"

void psh::ChatCharacter::MakeCreatePacket(SendBuffer& buffer, const bool spawn) const
{
    GameObject::MakeCreatePacket(buffer, spawn);
    MakeGame_ResChracterDetail(buffer, ObjectId(), _hp);
}

void psh::ChatCharacter::Attack(const SkillID type, const FVector dir)
{
    if (_attackStrategy == nullptr)
    {
        return;
    }

    auto& [_,skill] = *_skills.find(type);
    //만료 확인
    if(skill.timer.IsExpired() == false)
    {
        return;
    }

    ATTACK::attack({*this,dir,type,ATTACK::GetRangeBySkillID(Location(),Direction(),type)});

    //쿨타임 추가.
    skill.timer.Reset(Timer::ms{1000});
}

void psh::ChatCharacter::Hit(const DamageInfo info)
{
    //각종 데미지 계산 공식...
    _hp -= info.damage;

    auto hitPacket = SendBuffer::Alloc();
    MakeGame_ResHit(hitPacket, ObjectId(), info.attacker, _hp);

    for (const auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
         auto& player : view)
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
    auto buf = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(buf, ObjectId(), true, removeResult::Die);

    auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
    for (auto& player : _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST))
    {
        std::static_pointer_cast<Player>(player)->SendPacket(buf);
    }
    //printf("Die %d \n",ObjectId());
    _field.DestroyActor(shared_from_this());
}
