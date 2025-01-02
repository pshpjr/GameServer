#pragma once

#include <vector>
#include "FVector.h"
#include "ContentTypes.h"
#include "Range.h"

// 전방 선언
namespace psh
{
    class GameObject;
    class Range;
}

namespace psh
{
    struct DamageInfo
    {
        ObjectID attacker;
        int damage;
    };

    struct ReqAttack
    {
        const GameObject& attacker;
        FVector direction{};
        SkillID skillId{};
    };

    struct AttackInfo
    {
        eObjectType attackerType{};
        PoolPtr<Range> range{};
        SkillID skillId{};
        ObjectID attacker{};
        int damage{};
    };

    /**
     * 어떤 스킬의 범위, 데미지, ID
     * skillSize는 0,1 방향을 바라본 것을 기준으로 한다.
     */
    struct SkillInfo
    {
        SkillID id;
        FVector skillSize;
        int damage;
        int cooldown;
    };

    namespace ATTACK
    {
        // 공용(public) 멤버 함수
        /**
         * 공격 시스템 초기화
         */
        void Init();
        int GetSquarePoolSzie();
        int GetCirclePoolSzie();
        /**
         * 스킬 ID를 통해 스킬 정보를 가져옴
         * @param skillId 스킬 ID
         * @return 해당 스킬의 정보
         */
        SkillInfo GetSkillInfoById(SkillID skillId);

        /**
         * 객체 템플릿 ID를 바탕으로 스킬 목록을 반환
         * @param id 객체 템플릿 ID
         * @return 스킬 ID 목록
         */
        std::vector<SkillID>& GetSkillsByTemplate(TemplateID id);

        /**
         * 공격 실행 함수
         * @param attack 공격 요청 정보
         */
        void ExecuteAttack(ReqAttack attack);

        /**
         * 스킬 사용시 해당 위치와 방향에 맞는 범위를 계산
         * @param location 위치
         * @param dir 방향
         * @param id 스킬 ID
         * @return 스킬 범위
         */
        RangeUnique CalculateSkillRange(FVector location, FVector dir, SkillID id);

        /**
         * 아이템 ID를 통해 범위 객체를 가져옴
         * @param location
         * @param id 아이템 ID
         * @return 범위 객체
         */
        RangeUnique CalculateRangeByItemId(FVector location, TemplateID id);
    }
}
