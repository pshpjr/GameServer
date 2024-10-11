//
// Created by pshpj on 24. 10. 7.
//

#ifndef MONSTERAI_H
#define MONSTERAI_H
#include <ContentTypes.h>
#include <FVector.h>


namespace psh
{
    class ChatCharacter;
}

namespace psh
{
    class Field;
    class GameObject;
}

namespace psh::MonsterAi
{
    struct TargetRequest
    {
        FVector location;
        ObjectID objectId; //요청자
    };

    using TargetSelector = std::function<std::weak_ptr<GameObject>(TargetRequest, Field&)>;

    extern const TargetSelector PveTargetSelector;
    extern const TargetSelector PvpTargetSelector;

    TargetSelector GetTargetSelectorByType(ServerType type);
}


#endif //MONSTERAI_H
