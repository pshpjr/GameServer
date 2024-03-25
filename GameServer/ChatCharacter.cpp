#include "ChatCharacter.h"

#include "AttackManager.h"
#include "ObjectManager.h"
#include "TableData.h"
#include "AttackData.h"

#include "GroupCommon.h"

void psh::ChatCharacter::MakeCreatePacket(SendBuffer& buffer, bool spawn) const
{
    GameObject::MakeCreatePacket(buffer, spawn);
    MakeGame_ResChracterDetail(buffer,ObjectId(),_hp);
}

void psh::ChatCharacter::Attack(char type)
{
    if(_attackManager == nullptr)
        return;
    
    auto attackPacket = SendBuffer::Alloc();
    MakeGame_ResAttack(attackPacket, ObjectId(), type);

    _group.SendInRange(Location(),SEND_OFFSETS::BROADCAST, attackPacket);

    SquareRange attackRange = {
        {Location().X - _attacks[type].first.X / 2, Location().Y}
        , {Location().X + _attacks[type].first.X / 2, Location().Y + _attacks[type].first.Y}};
    attackRange.Rotate(Direction(), Location());

    auto draw = SendBuffer::Alloc();
    for (auto& point : attackRange._points)
    {
        MakeGame_ResDraw(draw, point);
    }
    _group.SendInRange(Location(),SEND_OFFSETS::BROADCAST, draw);
    _attackManager->OnAttack(AttackData{&attackRange, _attacks[type].second,ObjectId(),ObjectGroup()});
}

void psh::ChatCharacter::Hit(int damage, const psh::ObjectID attacker)
{
    _hp -= damage;

    auto hitPacket = SendBuffer::Alloc();
    MakeGame_ResHit(hitPacket, ObjectId(), attacker, _hp);

    _group.SendInRange(Location(),SEND_OFFSETS::BROADCAST, hitPacket);

    if (_hp <= 0)
    {
        Die();
    }
}

void psh::ChatCharacter::Die()
{
    SetNeedUpdate(false);
    _owner.RemoveFromMap(shared_from_this(),Location(),SEND_OFFSETS::BROADCAST,true,false, ObjectManager::removeResult::Die);
    //printf("Die %d \n",ObjectId());
    _owner.DestroyActor(this);
}
