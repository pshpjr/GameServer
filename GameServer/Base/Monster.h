#pragma once
#include "ChatCharacter.h"

namespace psh
{

    class Monster : public psh::ChatCharacter
    {
    public:
        ~Monster() override = default;
        void OnUpdate(float delta) override;
        void Die() override;
        void OnMove() override;
        void MoveStart(FVector destination) override;

    public:
	
        Monster(ObjectID clientId,const psh::FVector& location, const psh::FVector& direction,char type);

    };
    

}
