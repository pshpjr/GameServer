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

    struct DamageInfo {
        int damage;
        ObjectID attacker;
    };

    struct ReqAttack {
        const GameObject &attacker;
        FVector direction{};
        SkillID skillId{};
        PoolPtr<Range> range{};
    };

    struct AttackInfo {
        eObjectType attackerType{};
        PoolPtr<Range> range{};
        SkillID skillId{};
        ObjectID attacker{};
        int damage{};
    };

    /**
     * 어떤 스킬의 범위, 데미지, id
     * skillSize는 0,0방향을 바라본 것을 기준으로 한다.
     */
    struct SkillInfo {
        SkillID id;
        FVector skillSize;
        int damage;
    };

    namespace ATTACK
    {
        void Init();

        SkillInfo GetSkillInfoById(SkillID skillId);

        /**
         * id를 바탕으로 가지고 있는 스킬들을 리턴
         * @param id 객체 템플릿 id
         * @return
         */
        std::vector<SkillID> &GetSkillsByTemplate(TemplateID id);

        int GetAIRangeByTemplate(TemplateID id);

        void ExecuteAttack(ReqAttack attack);

        /**
         * 해당 위치에서 방향으로 스킬 썼을 때 맞는 범위를 리턴함. 다형성 위해 unique_ptr 리턴.
         * @param location
         * @param dir
         * @param id
         * @return
         */
        RangeUnique CalculateSkillRange(FVector location, FVector dir, SkillID id);

        RangeUnique GetRangeByItemID(TemplateID id);
    }
}
