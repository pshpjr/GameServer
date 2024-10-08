//
// Created by Park on 24. 8. 20.
//
#include "AttackData.h"

#include "Field.h"
#include "IVictimSelect.h"
#include "ModernObjectPool.h"
#include "Player.h"
#include "Range.h"
#include "TableData.h"

namespace psh
{
    namespace
    {
        inline HashMap<SkillID, SkillInfo> skillsMap;
        inline HashMap<TemplateID, std::vector<SkillID> > templateSkillsMap;

        inline std::vector<std::vector<std::pair<FVector, int> > > playerAttack;

        inline std::vector<std::vector<std::pair<FVector, int> > > monsterAttack;
    }
}

//나중엔 파일에서 데이터 동적으로 들고 온다.
void psh::ATTACK::Init()
{
    playerAttack =
    {
        {
            {{60, 60}, 4}
          , {{60, 60}, 6}
          , {{60, 60}, 8}
        }
      , {
            {{60, 60}, 4}
          , {{60, 65}, 6}
          , {{60, 60}, 8}
        }
      , {
            {{60, 60}, 5}
          , {{60, 65}, 6}
          , {{60, 60}, 7}
        }
      , {
            {{60, 60}, 5}
          , {{60, 63}, 6}
          , {{60, 61}, 7}
        }
    };

    monsterAttack =
    {
        {
            {{60, 60}, 1}
          , {{60, 60}, 2}
          , {{60, 60}, 3}
           ,
        }
      , {
            {{60, 60}, 1}
          , {{60, 60}, 2}
          , {{60, 60}, 3}
           ,
        }
      , {
            {{60, 60}, 8}
          , {{60, 60}, 7}
          , {{60, 60}, 6}
           ,
        }
      , {
            {{60, 60}, 6}
          , {{60, 60}, 10}
          , {{60, 60}, 4}
           ,
        }
       ,
    };
    char skillNum = 0;
    char templateNum = 0;
    for (auto &player: playerAttack)
    {
        for (auto &[size, damage]: player)
        {
            templateSkillsMap[templateNum].push_back(skillNum);
            skillsMap.emplace(skillNum, SkillInfo{skillNum, size, damage});
            ++skillNum;
        }
        ++templateNum;
    }

    for (auto &player: monsterAttack)
    {
        for (auto &[size, damage]: player)
        {
            templateSkillsMap[templateNum].push_back(skillNum);
            skillsMap.emplace(skillNum, SkillInfo{skillNum, size, damage});
            ++skillNum;
        }
        ++templateNum;
    }
    //빈 스킬. 공격 없는 애들 용.
    templateSkillsMap.insert({127, {}});
}

psh::SkillInfo psh::ATTACK::GetSkillInfoById(const SkillID skillId)
{
    //없는 스킬 사용하면 크래쉬.
    return skillsMap.find(skillId)->second;
}

std::vector<psh::SkillID> &psh::ATTACK::GetSkillsByTemplate(const TemplateID id)
{
    return templateSkillsMap[id];
}

int psh::ATTACK::GetAIRangeByTemplate(TemplateID id)
{
    //TODO: 테스트용
    return 60;
}


void psh::ATTACK::ExecuteAttack(ReqAttack attack)
{
    auto &attacker = attack.attacker;
    const auto &[id, skillSize, damage] = GetSkillInfoById(attack.skillId);
    auto range = CalculateSkillRange(attacker.Location(), attack.direction, attack.skillId);
    AttackInfo info{attacker.ObjectType(), std::move(range), attack.skillId, attacker.ObjectId(), damage};

    //공격패킷과 이펙트 패킷을 전송하기
    //나중에 miss등의 이펙트도 생길 수 있음.
    if (auto result = attacker.GetField().ProcessAttack(std::move(info));
        result == victim_select::AttackResult::Success)
    {
        auto attackPacket = SendBuffer::Alloc();
        MakeGame_ResAttack(attackPacket, attacker.ObjectId(), attack.skillId, attack.direction);

        attacker.GetField().BroadcastToPlayer(attacker.Location(), {attackPacket});
    }
}

namespace rangePool
{
    ModernObjectPool<psh::SquareRange> squarePool(100);
    ModernObjectPool<psh::CircleRange> circlePool(100);
}


psh::RangeUnique psh::ATTACK::CalculateSkillRange(FVector location, FVector dir, SkillID id)
{
    const auto &[_, skillSize, damage] = GetSkillInfoById(id);

    auto range = rangePool::squarePool.Alloc(FVector{location.X - skillSize.X / 2, location.Y}
                                           , FVector{location.X + skillSize.X / 2, location.Y + skillSize.Y});

    range->Rotate(dir, location);

    return rangePool::squarePool.Cast<Range>(std::move(range));
}

psh::RangeUnique psh::ATTACK::GetRangeByItemID(TemplateID id)
{
    auto range = rangePool::circlePool.Alloc(FVector{0, 0}, 30.0f);

    return rangePool::circlePool.Cast<Range>(std::move(range));
}
