#include "Monster.h"

#include "AttackData.h"
#include "Field.h"
#include "MoveComponent.h"
#include "Player.h"
#include "Profiler.h"
#include "RangeObject.h"
#include "AiComponent.h"

namespace psh
{
    const Nickname& GetMonsterName(TemplateID id)
    {
        static const std::array monsterNames = {
            Nickname("Invalid"), Nickname("Invalid"), Nickname("Invalid"), Nickname("Invalid"), // 0~3
            Nickname("Easy_1")
            , Nickname("Easy_2"), Nickname("Easy_3"), Nickname("Easy_4"), // 4~7
            Nickname("Hard_1")
            , Nickname("Hard_2"), Nickname("Hard_3"), Nickname("Hard_4"), // 8~11
            Nickname("PVP_1")
            , Nickname("PVP_2"), Nickname("PVP_3"), Nickname("PVP_4") // 12~15
        };

        if (id < 0 || id >= static_cast<int>(monsterNames.size()))
        {
            ASSERT_CRASH(false, "Invalid monster");
        }

        return monsterNames[id];
    }

    Monster::Monster(Field& group, const GameObjectData& initData, std::unique_ptr<MonsterAiComponent> aiComponent)
        : ChatCharacter(group, initData, std::move(aiComponent)) {}


    void Monster::MakeCreatePacket(SendBuffer& buffer, bool spawn) const
    {
        ChatCharacter::MakeCreatePacket(buffer, spawn);
        MakeGame_ResPlayerDetail(buffer, ObjectId(), GetMonsterName(TemplateId()));
    }

    void Monster::DieImpl()
    {
        GameObjectData itemData{Location(), {0, 0}, 0, eObjectType::Item, 16};

        const auto obj = std::make_shared<Item>(_field, itemData
                                                , ATTACK::CalculateRangeByItemID(
                                                    itemData.location, itemData.templateId));

        _field.AddActor(obj);
        _removeReason = removeReason::Die;
    }
}
