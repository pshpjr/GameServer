#pragma once
#include "ChatCharacter.h"

namespace psh
{
    class AiState;

    class Monster : public ChatCharacter
    {
    public:
        ~Monster() override
        {
            printf("del\n");
        }

        void OnUpdate(float delta) override;
    public:
        float attackCooldown = 0;
        float moveCooldown = 0;
        weak_ptr<ChatCharacter> _target;
  
        Monster(ObjectID clientId,ObjectManager& manager, GroupCommon& group, const FVector& location, char type);
    };
}
