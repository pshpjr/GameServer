#include "ChatCharacter.h"
#include "../Group/GroupCommon.h"

void psh::ChatCharacter::Attack(char type)
{
    auto attackPacket =  SendBuffer::Alloc();
    MakeGame_ResAttack(attackPacket, ObjectId(),type);
		
    _owner->Broadcast(Location(),attackPacket);
    
    SquareRange attackRange = {{Location().X- _attacks[type].first.X/2,Location().Y  },
        {Location().X + _attacks[type].first.X/2,Location().Y + _attacks[type].first.Y}};
    attackRange.Rotate(Direction(),Location());

    
    _owner->CheckVictim(attackRange, _attacks[type].second
        ,static_pointer_cast<ChatCharacter>(shared_from_this()));
}

void psh::ChatCharacter::Hit(int damage ,const shared_ptr<ChatCharacter>& attacker)
{
    _hp -= damage;
		
    auto hitPacket =  SendBuffer::Alloc();
    MakeGame_ResHit(hitPacket, ObjectId(),attacker->ObjectId(), _hp);
    _owner->Broadcast(Location(),hitPacket);

    if( _hp <= 0 )
    {
        Die();
    }
}

void psh::ChatCharacter::MoveStart(FVector destination)
{
    GameObject::MoveStart(destination);
    auto movePacket =  SendBuffer::Alloc();
    MakeGame_ResMove(movePacket, ObjectId(),destination);
    
    _owner->Broadcast(Location(),movePacket);
}

void psh::ChatCharacter::OnMove()
{
    _owner->BroadcastMove(static_pointer_cast<ChatCharacter>(shared_from_this())
        ,OldLocation(),Location());
}

void psh::ChatCharacter::Die()
{
    Destroy(true);
}
