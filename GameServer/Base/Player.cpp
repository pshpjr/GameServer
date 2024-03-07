#include "stdafx.h"
#include "Player.h"

#include <PacketGenerated.h>

#include "IOCP.h"

#include "../GameMap.h"
#include "../Data/AttackData.h"


namespace psh 
{
	constexpr int PLAYER_MOVE_SPEED = 400;
	
	Player::Player(ObjectID clientId, const psh::FVector& location, const psh::FVector& direction, char type,
		const SessionID& sessionId, AccountNo accountNo)
	: ChatCharacter(clientId,location,direction,PLAYER_MOVE_SPEED,eCharacterGroup::Player,type)
	,_sessionId(sessionId),_accountNo(accountNo)
	{
		_attacks = playerAttack[type];
	}
	
	void Player::OnUpdate(float delta)
	{
	}

	void Player::Die()
	{
		ChatCharacter::Die();
	}
	
	void Player::OnMove()
	{
		ChatCharacter::OnMove();
		_owner->CheckItem(static_pointer_cast<ChatCharacter>(shared_from_this()));
	}
}

