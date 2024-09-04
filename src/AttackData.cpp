//
// Created by Park on 24. 8. 20.
//
#include "AttackData.h"

#include "Player.h"
#include "TableData.h"
#include "Field.h"
#include "IVictimSelect.h"
#include "ModernObjectPool.h"
#include "Range.h"

namespace psh
{
    namespace
    {
        inline static HashMap<SkillID,SkillInfo> skillsMap;
        inline static HashMap<TemplateID,std::vector<SkillID>> templateSkillsMap;

        inline static std::vector<std::vector<std::pair<FVector, int>>> playerAttack;

        inline static std::vector<std::vector<std::pair<FVector, int>>> monsterAttack;
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
    for(auto& player: playerAttack)
    {
        for(auto&[size, damage] : player)
        {
            templateSkillsMap[templateNum].push_back(skillNum);
            skillsMap.emplace(skillNum,SkillInfo{skillNum,size,damage});
            ++skillNum;
        }
        ++templateNum;
    }

    for(auto& player: monsterAttack)
    {
        for(auto& [size, damage]  : player)
        {
            templateSkillsMap[templateNum].push_back(skillNum);
            skillsMap.emplace(skillNum,SkillInfo{skillNum,size,damage});
            ++skillNum;
        }
        ++templateNum;
    }
    //빈 스킬. 공격 없는 애들 용.
    templateSkillsMap.insert({127,{}});
}

psh::SkillInfo psh::ATTACK::GetSkillInfoById(const SkillID skillId)
{
    //없는 스킬 사용하면 크래쉬.
    return skillsMap.find(skillId)->second;
}

std::vector<psh::SkillID> & psh::ATTACK::GetSkillsByTemplate(const TemplateID id)
{
    return templateSkillsMap[id];
}

int psh::ATTACK::GetAIRangeByTemplate(TemplateID id)
{

    //TODO: 테스트용
    return 60;
}

void psh::ATTACK::attack(ReqAttack attack)
{
    //공격패킷과 이펙트 패킷을 전송하기.
    const auto& skill = ATTACK::GetSkillInfoById(attack.skillId);

    auto& attacker = attack.attacker;

    auto attackPacket = SendBuffer::Alloc();
    MakeGame_ResAttack(attackPacket, attacker.ObjectId(), attack.skillId, attack.direction);
    
    auto draw = SendBuffer::Alloc();
    attack.range->DrawRangeIntoBuffer(draw);

    for (const auto view = attacker.GetField().GetPlayerView(attacker.Location(), SEND_OFFSETS::BROADCAST);
         auto& player : view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(attackPacket);
        std::static_pointer_cast<Player>(player)->SendPacket(draw);
    }

    AttackInfo info{attacker.ObjectType(),nullptr,attack.skillId,attacker.ObjectId(),skill.damage};

    info.range = std::move(attack.range);
    attacker.GetField().GetVictimSelect()->GetVictim(info);




    //_attackStrategy->Attack(AttackInfo{&attackRange, _skills[type].second,ObjectId(),ObjectType()});
}

namespace rangePool{
    ModernObjectPool<psh::SquareRange> squarePool(100);
    ModernObjectPool<psh::CircleRange> circlePool(100);

}


psh::RangeUnique psh::ATTACK::GetRangeBySkillID(FVector location, FVector dir, SkillID id)
{

    const auto& skill = ATTACK::GetSkillInfoById(id);

    auto range = rangePool::squarePool.Alloc(FVector{location.X - skill.skillSize.X / 2, location.Y}
        , FVector{location.X + skill.skillSize.X / 2, location.Y + skill.skillSize.Y});

    range->Rotate(dir, location);

    return rangePool::squarePool.Cast<Range>(std::move(range));
}

psh::RangeUnique psh::ATTACK::GetRangeByItemID(TemplateID id)
{
    auto range = rangePool::circlePool.Alloc(FVector{0,0},30.0f);

    return rangePool::circlePool.Cast<Range>(std::move(range));
}
