#include "IVictimSelect.h"

#include "ChatCharacter.h"
#include "Field.h"
#include "TableData.h"
#include "Macro.h"

namespace psh::victim_select
{
    VictimSelectFunction pveVictimSelector = [](psh::Field &field, const psh::AttackInfo &attackInfo) {
        //어떤 범위에 고유 점들을 가지고 오고, 이걸 섹터 리스트로 변환하고, 이 섹터 리스트에 대해 뷰를 받아와야 함.
        //range를 받으면


        switch (attackInfo.attackerType)
        {
            case eObjectType::Player:
            {
                // for (auto view = field.GetMonsterView();
                //      auto &obj: view)
                // {
                //     if (attackInfo.range->Contains(obj->Location()))
                //     {
                //         std::static_pointer_cast<ChatCharacter>(obj)->Hit({attackInfo.attacker, attackInfo.damage});
                //     }
                // }
            }
            break;
            case eObjectType::Monster:
            {
                auto view = field.GetPlayerViewByCoordinate(attackInfo.range->GetCoordinates());

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

    VictimSelectFunction psh::victim_select::GetVictimByServerType(ServerType type)
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

