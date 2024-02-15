#pragma once
#include "ContentTypes.h"
#include "CRecvBuffer.h"
#include "GameMap.h"


//player는 어떤 값이 올바른지 모르니까 외부에서 올바른 값으로 넣어줘야 하는가?
//player는 map에서만 생성하는가?

namespace psh 
{
	struct Sector;
	class Player
	{
		//friend Sector GameMap::CalculatePlayerSector(const Player& player);
	public:
		Player(const psh::FVector& location, const psh::FVector& direction, const psh::FVector& vector,
			const SessionID& sessionId, AccountNo accountNo);

		//Player();
		void Attack();
		void Move();
		void Die();
	
	private:
		psh::FVector _location;

	public:
		void Location(psh::FVector newLocation)
		{
			_location = newLocation;
		}
		
		[[nodiscard]] psh::FVector Location() const
		{
			return _location;
		}

		[[nodiscard]] psh::FVector Direction() const
		{
			return _direction;
		}

		[[nodiscard]] psh::FVector Vector() const
		{
			return _vector;
		}

		[[nodiscard]] AccountNo AccountNumber() const
		{
			return _accountNo;
		}

	private:
		psh::FVector _direction;
		psh::FVector _vector;
		
		SessionID _sessionId;
		AccountNo _accountNo;
		
	public:
		[[nodiscard]] SessionID SessionId() const
		{
			return _sessionId;
		}
	};
}

