#include "stdafx.h"
#include "Player.h"


namespace psh 
{
	Player::Player(const psh::FVector& location, const psh::FVector& direction, const psh::FVector& vector,
	const SessionID& sessionId, AccountNo accountNo)
		: _location(location),
		  _direction(direction),
		  _vector(vector),
		  _sessionId(sessionId),
		  _accountNo(accountNo)
	{
	}


	// Player::Player(): _location(), _direction(), _vector(), _sessionId(), _accountNo(0)
	// {
	// }

	void Player::Attack()
	{
	}

	void Player::Move()
	{
	}

	void Player::Die()
	{
	}

}

