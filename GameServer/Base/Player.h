#pragma once
#include <memory>

#include "ContentTypes.h"
#include "ChatCharacter.h"


//player�� � ���� �ùٸ��� �𸣴ϱ� �ܺο��� �ùٸ� ������ �־���� �ϴ°�?
//player�� map������ �����ϴ°�?

namespace psh 
{
	
	struct Sector;
	
	class Player : public ChatCharacter
	{
	public:
		~Player() override = default;
		void OnUpdate(float delta) override;
		void Die() override;

	protected:
		void OnMove() override;

	public:
		
		Player(ObjectID clientId,const psh::FVector& location, const psh::FVector& direction,char type,
			const SessionID& sessionId, AccountNo accountNo );
		void GetCoin()
		{
			printf("%d \n",_coin);
			_coin++;
		}
	public:
		[[nodiscard]] SessionID SessionId() const
		{
			return _sessionId;
		}


		[[nodiscard]] AccountNo AccountNumber() const
		{
			return _accountNo;
		}

	private:
		SessionID _sessionId;
		AccountNo _accountNo;
		int _coin = 0;
	};
}

