#include "stdafx.h"
#include "Player.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "../GameMap.h"


namespace psh 
{
	constexpr int PLAYER_MOVE_SPEED = 400;
	
	Player::Player(ObjectID clientId, const psh::FVector& location, const psh::FVector& direction, char type,
		const SessionID& sessionId, AccountNo accountNo)
	: ChatCharacter(clientId,location,direction,PLAYER_MOVE_SPEED,eCharacterGroup::Player,type)
	,_sessionId(sessionId),_accountNo(accountNo)
	{
	}

	void Player::Attack(char type)
	{
		auto attackPacket =  SendBuffer::Alloc();
		MakeGame_ResAttack(attackPacket, ObjectId(),type);
		
		_owner->Broadcast(Location(),attackPacket);

		SquareRange attackRange = {{Location().X- _attacks[type].first.X/2,Location().Y  },
			{Location().X + _attacks[type].first.X/2,Location().Y + _attacks[type].first.Y}};
		attackRange.Rotate(Direction(),Location());

		auto draw =  SendBuffer::Alloc();
		for(auto& point : attackRange._points )
		{
			MakeGame_ResDraw(draw,point);
		}
		_map->Broadcast(Location(),draw);

		_map->CheckVictim(attackRange, _attacks[type].second,shared_from_this());
		
	}

	void Player::Hit(int damage)
	{
		_hp -= damage;
		
		auto hitPacket =  SendBuffer::Alloc();
		MakeGame_ResHit(hitPacket, ObjectId(),_hp);
		_map->Broadcast(Location(),hitPacket);

		if(_hp <=0)
			Die();
	}

	void Player::OnUpdate(float delta)
	{
	}

	void Player::Die()
	{

	}

	void Player::OnMove()
	{
		_map->BroadcastIfSectorChange(shared_from_this(),OldLocation(),Location());
	}

	void Player::MoveStart(FVector destination)
	{
		ChatCharacter::MoveStart(destination);

		auto movePacket =  SendBuffer::Alloc();
		MakeGame_ResMove(movePacket, ObjectId(),destination);
		
		_map->Broadcast(Location(),movePacket);
	}

	void Player::SendPacket(IOCP* iocp, SendBuffer& buffer)
	{
		iocp->SendPacket(_sessionId,buffer);
	}
}

