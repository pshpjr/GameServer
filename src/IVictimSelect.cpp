#include "IVictimSelect.h"

#include "AttackData.h"
#include "ChatCharacter.h"
#include "Field.h"

void psh::PveVictim::GetVictim(const AttackInfo &attackInfo) const
{
    switch (attackInfo.type) {
        case eObjectType::Player:
        {
            for(auto view = _field->GetItemViewByList(attackInfo.range->GetCoordinates());
                auto& obj : view)
            {
                if(attackInfo.range->Contains(obj->Location()))
                {
                    std::static_pointer_cast<ChatCharacter>(obj)->Hit({attackInfo.attacker,attackInfo.damage});
                }
            }
        }
             break;
        case eObjectType::Monster:
        {
            auto view = _field->GetItemViewByList(attackInfo.range->GetCoordinates());

        }
            break;
        case eObjectType::Item:
            [[fallthrough]];
        case eObjectType::Object:
            [[fallthrough]];
        default:
            ASSERT_CRASH(false, "InvalidType");
    }
}

void psh::PvpVictim::GetVictim(const AttackInfo &attackInfo) const
{
    // switch (attackInfo.type) {
    //     case eObjectType::Player:
    //         [[fallthrough]]
    //     case eObjectType::Monster:
    //         //auto view = ranges::views::concat(_field->GetMonsterView(attackInfo.range->GetCoordinates()),_field->GetPlayerView());
    //     break;
    //     case eObjectType::Item:
    //         [[fallthrough]]
    //     case eObjectType::Object:
    //         [[fallthrough]]
    //     case default:
    //         ASSERT_CRASH(false, "InvalidType");
    // }
}

std::unique_ptr<psh::IVictimSelect> psh::victim_select::GetVictimByServerType(ServerType type, Field &field)
{
    switch (type)
    {
    case ServerType::Village:
        return nullptr;
        break;
    case ServerType::Easy:
        return std::make_unique<PveVictim>(&field);
        break;
    case ServerType::Hard:
        return std::make_unique<PveVictim>(&field);
        break;
    case ServerType::Pvp:
        return std::make_unique<PvpVictim>(&field);
        break;
    case ServerType::End:
        ASSERT_CRASH(false, "invalid state");
        return nullptr;
        break;
    }
    return nullptr;
}
