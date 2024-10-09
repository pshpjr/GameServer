#include "IVictimSelect.h"

#include "ChatCharacter.h"
#include "Field.h"
#include "Macro.h"

namespace psh::victim_select
{
    VictimSelectFunction pveVictimSelector = [](psh::Field &field, const psh::AttackInfo &attackInfo) {
        switch (attackInfo.attackerType)
        {
            case eObjectType::Player:
            {
                for (auto view = field.GetObjectViewByPoint(psh::Field::ViewObjectType::Monster
                                                          , attackInfo.range->GetCoordinates());
                     auto &obj: view)
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
                auto view = field.GetObjectViewByPoint(psh::Field::ViewObjectType::Player
                                                     , attackInfo.range->GetCoordinates());

                for (auto &obj: view)
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


    VictimSelectFunction pvpVictimSelector;

    VictimSelectFunction invalidSelector = [](psh::Field &field, const psh::AttackInfo &attackInfo) {
        return AttackResult::Invalid;
    };

    VictimSelectFunction psh::victim_select::GetVictimByServerType(psh::ServerType type)
    {
        switch (type)
        {
            case psh::ServerType::Village:
                return invalidSelector;
                break;
            case psh::ServerType::Easy:
                [[fallthrough]];
            case psh::ServerType::Hard:
                return pveVictimSelector;
                break;
            case psh::ServerType::Pvp:
                return pvpVictimSelector;
                break;
            case psh::ServerType::End:
                ASSERT_CRASH(false, "invalid state");
                return nullptr;
                break;
        }
        return nullptr;
    }
}

