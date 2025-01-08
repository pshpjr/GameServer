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
    const TargetSelector NoBattleTargetSelector = [](TargetRequest request, Field& field)-> std::weak_ptr<GameObject>
    {
        return {};
    };


    const TargetSelector PveTargetSelector = [](TargetRequest request, Field& field)-> std::weak_ptr<GameObject>
    {
        float curMax = 1000;
        std::weak_ptr<GameObject> target;

        auto view = field.GetObjectView(Field::ViewObjectType::Player, request.location, SEND_OFFSETS::BROADCAST);
        for (const auto& obj : view)
        {
            if (Distance(obj->Location(), request.location) < curMax)
            {
                curMax = Distance(obj->Location(), request.location);
                target = obj;
            }
        }
        return target;
    };

    const TargetSelector PvpTargetSelector = [](TargetRequest request, Field& field)-> std::weak_ptr<GameObject>
    {
        float curMax = 1000;
        std::weak_ptr<GameObject> target;

        for (auto view = field.GetObjectView(Field::ViewObjectType::Player, request.location, SEND_OFFSETS::BROADCAST);
             const auto& obj : view)
        {
            if (Distance(obj->Location(), request.location) < curMax)
            {
                curMax = Distance(obj->Location(), request.location);
                target = obj;
            }
        }

        for (auto view = field.GetObjectView(Field::ViewObjectType::Monster, request.location, SEND_OFFSETS::BROADCAST);
             const auto& obj : view)
        {
            if (obj->ObjectId() == request.objectId)
            {
                continue;
            }


            if (Distance(obj->Location(), request.location) < curMax)
            {
                curMax = Distance(obj->Location(), request.location);
                target = obj;
            }
        }

        return target;
    };

    TargetSelector GetTargetSelectorByType(ServerType type)
    {
        switch (type)
        {
        case ServerType::Pvp:
            return PvpTargetSelector;
        case ServerType::End:
        case ServerType::Hard:
            return PveTargetSelector;

        default: ;
        }
        return NoBattleTargetSelector;
    }
}
