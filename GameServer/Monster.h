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
        }

        void OnUpdate(int delta) override;
        
    public:
        FVector _spawnLocation = {0,0};
        float attackCooldown = 0;
        float moveCooldown = 0;
        float searchCooldown = 0;
        weak_ptr<ChatCharacter> _target;
  
        Monster(ObjectID clientId,ObjectManager& manager, GroupCommon& group, const FVector& location, char type);
    };
}
