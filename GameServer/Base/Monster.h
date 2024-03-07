#pragma once
#include "ChatCharacter.h"

namespace psh
{
    class AiState;

    class Monster : public psh::ChatCharacter
    {
    public:
        ~Monster() override {printf("del\n");}
        void OnUpdate(float delta) override;
        
        void Die() override;
        void OnMove() override;
        void MoveStart(FVector destination) override;

    public:
        int attackCooldown = 0;
        int moveCooldown = 0;
	    AiState* _state;
        shared_ptr<ChatCharacter> _target;
        Monster(ObjectID clientId,const psh::FVector& location, const psh::FVector& direction,char type);

    };

    
}
