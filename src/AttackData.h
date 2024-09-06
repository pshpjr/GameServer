#pragma once
#include <vector>
#include "FVector.h"
#include "GameObject.h"
#include "Range.h"

namespace psh
{
    class Range;
}

namespace psh
{
    using SkillID = char;
    struct DamageInfo
    {
        int damage;
        ObjectID attacker;
    };

    struct ReqAttack
    {
        const GameObject& attacker;
        FVector direction{};
        SkillID skillId{};
        PoolPtr<Range> range{};
    };

    struct AttackInfo
    {
        eObjectType type{};
        PoolPtr<Range> range{};
        SkillID skillId{};
        ObjectID attacker{};
        int damage{};
    };

    struct SkillInfo
    {
        SkillID id;
        FVector skillSize;
        int damage;
    };

    namespace ATTACK
    {
        void Init();

        SkillInfo GetSkillInfoById(SkillID skillId);

        std::vector<SkillID>& GetSkillsByTemplate(TemplateID id);
        int GetAIRangeByTemplate(TemplateID id);

        void attack(ReqAttack attack);

        RangeUnique GetRangeBySkillID(FVector location, FVector dir, SkillID id);
        RangeUnique GetRangeByItemID(TemplateID id);
    }
}
