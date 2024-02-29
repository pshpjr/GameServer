#include "Monster.h"
#include "../GameMap.h"
namespace psh 
{
    constexpr int PLAYER_MOVE_SPEED = 400;
	
    Monster::Monster(ObjectID clientId, const psh::FVector& location, const psh::FVector& direction, char type,
        const SessionID& sessionId, AccountNo accountNo)
    : ChatCharacter(clientId,location,direction,PLAYER_MOVE_SPEED,eCharacterGroup::Player,type)
    ,_sessionId(sessionId),_accountNo(accountNo)
    {
    }

    void Monster::Attack(char type)
    {
        auto attackPacket =  SendBuffer::Alloc();
        MakeGame_ResAttack(attackPacket, ObjectId(),type);
		
        _map->Broadcast(Location(),attackPacket);

        SquareRange attackRange = {{Location().X- _attacks[type].first.X/2,Location().Y  },
            {Location().X + _attacks[type].first.X/2,Location().Y + _attacks[type].first.Y}};
        attackRange.Rotate(Direction(),Location());


        _map->CheckVictim(attackRange, _attacks[type].second,shared_from_this());
		
    }

    void Monster::Hit(int damage)
    {
        _hp -= damage;
		
        auto hitPacket =  SendBuffer::Alloc();
        MakeGame_ResHit(hitPacket, ObjectId(),_hp);
        _map->Broadcast(Location(),hitPacket);

        if(_hp <=0)
            Die();
    }

    void Monster::OnUpdate(float delta)
    {
    }

    void Monster::Die()
    {

    }

    void Monster::OnMove()
    {
        _map->BroadcastIfSectorChange(shared_from_this(),OldLocation(),Location());
    }

    void Monster::MoveStart(FVector destination)
    {
        ChatCharacter::MoveStart(destination);

        auto movePacket =  SendBuffer::Alloc();
        MakeGame_ResMove(movePacket, ObjectId(),destination);
		
        _map->Broadcast(Location(),movePacket);
    }
}
