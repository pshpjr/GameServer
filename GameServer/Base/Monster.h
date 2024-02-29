#pragma once
#include "ChatCharacter.h"

namespace psh
{

    class Monster : public psh::ChatCharacter ,public enable_shared_from_this<Monster>
    {
    public:
        ~Monster() override = default;
        void Attack(char type) override;
        void Hit(int damage) override;
        void OnUpdate(float delta) override;
        void Die() override;
        void OnMove() override;
        void MoveStart(FVector destination) override;

    public:
	
        Monster(ObjectID clientId,const psh::FVector& location, const psh::FVector& direction,char type,
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
