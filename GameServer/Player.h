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
		void Die();
		void Move(FVector delta);
	private:


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

		void Destination(psh::FVector destination)
		{
			_destination = destination;
		}
		[[nodiscard]] psh::FVector Destination() const
		{
			return _destination;
		}

		[[nodiscard]] AccountNo AccountNumber() const
		{
			return _accountNo;
		}
		[[nodiscard]] bool isMove() const
		{
			return _move;
		}

		void MoveStart()
		{
			_move = true;
		}
		
		void MoveStop()
		{
			_move = false;
		}
	private:
		psh::FVector _location;
		psh::FVector _direction;
		psh::FVector _destination;
		
		SessionID _sessionId;
		AccountNo _accountNo;

		bool _move = false;
		
	public:
		[[nodiscard]] SessionID SessionId() const
		{
			return _sessionId;
		}
	};
}

