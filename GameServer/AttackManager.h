#pragma once
#include <memory>

#include "FVector.h"

struct AttackData;

namespace psh
{
    class ChatCharacter;

    class AttackManager
    {
    public:
        virtual ~AttackManager() = default;
        virtual void OnAttack(const AttackData& attack) {}
        virtual bool GetClosestTarget(FVector location, weak_ptr<ChatCharacter>& target, float maxDist){return false;}
    }; 
}

