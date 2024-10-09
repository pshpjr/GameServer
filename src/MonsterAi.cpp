//
// Created by pshpj on 24. 10. 7.
//

#include "MonsterAi.h"

#include "ChatCharacter.h"
#include "Field.h"
#include "GameObject.h"
#include "TableData.h"

namespace psh::MonsterAi
{
    const TargetSelector PveTargetSelector = [](FVector location, Field *field)-> std::weak_ptr<ChatCharacter> {
        float curMax = 1000;
        std::weak_ptr<ChatCharacter> target;

        auto view = field->GetObjectView(Field::ViewObjectType::Player, location, SEND_OFFSETS::BROADCAST);
        for (const auto &obj: view)
        {
            if (Distance(obj->Location(), location) < curMax)
            {
                curMax = Distance(obj->Location(), location);
                target = std::static_pointer_cast<ChatCharacter>(obj);
            }
        }
        return target;
    };

    const TargetSelector PvpTargetSelector = [](FVector location, Field *field)-> std::weak_ptr<ChatCharacter> {
        float curMax = 1000;
        std::weak_ptr<ChatCharacter> target;


        for (auto view = field->GetObjectView(Field::ViewObjectType::Player, location, SEND_OFFSETS::BROADCAST);
             const auto &obj: view)
        {
            if (Distance(obj->Location(), location) < curMax)
            {
                curMax = Distance(obj->Location(), location);
                target = std::static_pointer_cast<ChatCharacter>(obj);
            }
        }

        for (auto view = field->GetObjectView(Field::ViewObjectType::Monster, location, SEND_OFFSETS::BROADCAST);
             const auto &obj: view)
        {
            if (Distance(obj->Location(), location) < curMax)
            {
                curMax = Distance(obj->Location(), location);
                target = std::static_pointer_cast<ChatCharacter>(obj);
            }
        }

        return target;
    };
}

