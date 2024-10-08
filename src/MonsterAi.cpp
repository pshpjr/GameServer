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

        for (auto &obj: field->GetPlayerView(location, SEND_OFFSETS::BROADCAST))
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

