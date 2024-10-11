//
// Created by Park on 24. 8. 20.
//

// 필요한 헤더 파일 포함
#include "AttackData.h"
#include "Field.h"
#include "IVictimSelect.h"
#include "ModernObjectPool.h"
#include "Player.h"
#include "Range.h"
#include "TableData.h"

namespace psh
{
    // 전역 상수 및 전역 변수
    static HashMap<SkillID, SkillInfo> skillsMap; // 스킬 정보 저장용 맵
    static HashMap<TemplateID, std::vector<SkillID>> templateSkillsMap; // 템플릿 별 스킬 ID 저장용 맵
    static std::vector<std::vector<std::pair<FVector, int>>> playerAttack; // 플레이어 공격 데이터
    static std::vector<std::vector<std::pair<FVector, int>>> monsterAttack; // 몬스터 공격 데이터

    // 함수 및 클래스의 정의
    void ATTACK::Init()
    {
        //TODO: 나중엔 파일에서 읽어오기
        // 플레이어 공격 데이터 초기화

        skillsMap = {
            {0, {0, {60, 30}, 12, 300}}
            , {1, {1, {65, 30}, 12, 300}}
            , {2, {2, {70, 30}, 12, 300}}
            , {3, {3, {40, 80}, 12, 300}}
            , {4, {4, {45, 85}, 12, 300}}
            , {5, {5, {45, 80}, 12, 300}}
            , {6, {6, {60, 60}, 12, 300}}
            , {7, {7, {60, 65}, 12, 300}}
            , {8, {8, {60, 60}, 12, 300}}
            , {9, {9, {80, 80}, 10, 300}}
            , {10, {10, {80, 80}, 10, 300}}
            , {11, {11, {80, 80}, 10, 300}}

            //몬스터용 Easy
            , {12, {12, {60, 60}, 4, 1000}}
            , {13, {13, {60, 60}, 4, 1000}}
            , {14, {14, {60, 60}, 4, 1000}}
            , {15, {15, {60, 60}, 4, 1000}}

            //몬스터 Hard
            , {16, {16, {60, 60}, 6, 1000}}
            , {17, {17, {60, 60}, 6, 1000}}
            , {18, {18, {60, 60}, 6, 1000}}
            , {19, {19, {60, 60}, 6, 1000}}

            //몬스터 Pvp
            , {20, {20, {60, 60}, 8, 1000}}
            , {21, {21, {60, 60}, 8, 1000}}
            , {22, {22, {60, 60}, 8, 1000}}
            , {23, {23, {60, 60}, 8, 1000}}
        };

        templateSkillsMap = {
            {0, {0, 1, 2}}
            , {1, {3, 4, 5}}
            , {2, {6, 7, 8}}
            , {3, {9, 10, 11}}

            //몬스터 easy
            , {4, {12}}
            , {5, {13}}
            , {6, {14}}
            , {7, {15}}

            //
            , {8, {16}}
            , {9, {17}}
            , {10, {18}}
            , {11, {19}}

            //
            , {12, {20}}
            , {13, {21}}
            , {14, {22}}
            , {15, {23}}
        };

        // 빈 스킬. 공격 없는 애들 용.
        templateSkillsMap.insert({127, {}});
    }

    SkillInfo ATTACK::GetSkillInfoById(const SkillID skillId)
    {
        // 없는 스킬 사용하면 크래쉬.
        return skillsMap.find(skillId)->second;
    }

    std::vector<SkillID>& ATTACK::GetSkillsByTemplate(const TemplateID id)
    {
        return templateSkillsMap[id];
    }

    void ATTACK::ExecuteAttack(ReqAttack attack)
    {
        auto& attacker = attack.attacker;
        const auto& [id, skillSize, damage,notUse] = GetSkillInfoById(attack.skillId);
        auto range = CalculateSkillRange(attacker.Location(), attack.direction, attack.skillId);
        AttackInfo info{attacker.ObjectType(), std::move(range), attack.skillId, attacker.ObjectId(), damage};
        if (attack.skillId == 12)
        {
            std::cout << "Attack12 :" << damage << std::endl;
        }

        // 공격패킷과 이펙트 패킷을 전송하기
        // 나중에 miss등의 이펙트도 생길 수 있음.
        if (auto result = attacker.GetField().ProcessAttack(std::move(info));
            result == AttackResult::Success)
        {
            auto attackPacket = SendBuffer::Alloc();
            MakeGame_ResAttack(attackPacket, attacker.ObjectId(), attack.skillId, attack.direction);
            attacker.GetField().BroadcastToPlayer(attacker.Location(), {attackPacket});
        }
    }

    // 기타 내부 함수 및 헬퍼 함수
    namespace rangePool
    {
        ModernObjectPool<SquareRange> squarePool(100);
        ModernObjectPool<CircleRange> circlePool(100);
    }

    RangeUnique ATTACK::CalculateSkillRange(FVector location, FVector dir, SkillID id)
    {
        const auto& [_, skillSize, damage,notUse] = GetSkillInfoById(id);
        auto range = rangePool::squarePool.Alloc(FVector{location.X - skillSize.X / 2, location.Y}
                                                 , FVector{location.X + skillSize.X / 2, location.Y + skillSize.Y});
        range->Rotate(dir, location);
        return rangePool::squarePool.Cast<Range>(std::move(range));
    }

    RangeUnique ATTACK::CalculateRangeByItemID(FVector location, TemplateID id)
    {
        auto range = rangePool::circlePool.Alloc(location, 30.0f);
        return rangePool::circlePool.Cast<Range>(std::move(range));
    }
} // 네임스페이스 정리
