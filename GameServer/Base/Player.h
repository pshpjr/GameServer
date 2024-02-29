#pragma once
#include <memory>

#include "ContentTypes.h"
#include "ChatCharacter.h"


//player는 어떤 값이 올바른지 모르니까 외부에서 올바른 값으로 넣어줘야 하는가?
//player는 map에서만 생성하는가?

namespace psh 
{
	
	struct Sector;
	
	class Player : public ChatCharacter ,public enable_shared_from_this<Player>
	{
	public:
		~Player() override = default;
		void Attack(char type) override;
		void Hit(int damage) override;
		void OnUpdate(float delta) override;
		void Die() override;
		void OnMove() override;
		void MoveStart(FVector destination) override;
		void SendPacket(IOCP* iocp, SendBuffer& buffer) override;
	public:
		
		Player(ObjectID clientId,const psh::FVector& location, const psh::FVector& direction,char type,
			const SessionID& sessionId, AccountNo accountNo );
	
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
		
		vector<pair<FVector,int>> _attacks =
		{
			{{200,200},10},
			{{200,200},20},			
			{{400,200},40},	
			{{500,200},10},	
		};
	};
}

