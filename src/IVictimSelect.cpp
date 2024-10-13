#include "IVictimSelect.h"

#include "ChatCharacter.h"
#include "Field.h"
#include "Macro.h"

namespace psh::victim_select
{
    VictimSelectFunction pveVictimSelector = [](Field& field, const AttackInfo& attackInfo) {
        switch (attackInfo.attackerType)
        {
        case eObjectType::Player:
            {
                for (auto view = field.GetObjectViewByPoint(Field::ViewObjectType::Monster
                                                            , attackInfo.range->GetCoordinates());
                     auto& obj : view)
                {
                    if (attackInfo.range->Contains(obj->Location()))
                    {
                        std::static_pointer_cast<ChatCharacter>(obj)->Hit({attackInfo.attacker, attackInfo.damage});
                    }
                }
            }
            break;
        case eObjectType::Monster:
            {
                auto view = field.GetObjectViewByPoint(Field::ViewObjectType::Player
                                                       , attackInfo.range->GetCoordinates());

                for (auto& obj : view)
                {
                    if (attackInfo.range->Contains(obj->Location()))
                    {
                        std::static_pointer_cast<ChatCharacter>(obj)->Hit({attackInfo.attacker, attackInfo.damage});
                    }
                }
            }
            break;
        case eObjectType::Item:
            [[fallthrough]];
        case eObjectType::Object:
            [[fallthrough]];
        default:
            ASSERT_CRASH(false, "InvalidType");
        }
        return AttackResult::Success;
    };


    VictimSelectFunction pvpVictimSelector = [](Field& field, const AttackInfo& attackInfo) {
        //몬스터
        {
            for (auto view = field.GetObjectViewByPoint(Field::ViewObjectType::Monster
                                                        , attackInfo.range->GetCoordinates());
                 auto& obj : view)
            {
                if (attackInfo.range->Contains(obj->Location()))
                {
                    std::static_pointer_cast<ChatCharacter>(obj)->Hit({attackInfo.attacker, attackInfo.damage});
                }
            }
        }
        // 플레이어 전부 맞음
        {
            auto view = field.GetObjectViewByPoint(Field::ViewObjectType::Player
                                                   , attackInfo.range->GetCoordinates());

            for (auto& obj : view)
            {
                if (attackInfo.range->Contains(obj->Location()))
                {
                    std::static_pointer_cast<ChatCharacter>(obj)->Hit({attackInfo.attacker, attackInfo.damage});
                }
            }
        }

        if (attackInfo.attackerType != eObjectType::Player && attackInfo.attackerType != eObjectType::Monster)
        {
            ASSERT_CRASH(false, "InvalidType");
        }


        return AttackResult::Success;
    };

    VictimSelectFunction invalidSelector = [](Field& field, const AttackInfo& attackInfo) {
        //ASSERT_CRASH(false, "Invalid");
        return AttackResult::Invalid;
    };

    VictimSelectFunction victim_select::GetVictimByServerType(ServerType type)
    {
        switch (type)
        {
        case ServerType::Village:
            return invalidSelector;
            break;
        case ServerType::Easy:
            [[fallthrough]];
        case ServerType::Hard:
            return pveVictimSelector;
            break;
        case ServerType::Pvp:
            return pvpVictimSelector;
            break;
        case ServerType::End:
            ASSERT_CRASH(false, "invalid state");
            return nullptr;
            break;
        }
        return nullptr;
    }
}

