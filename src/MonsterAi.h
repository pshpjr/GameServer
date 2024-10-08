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
    using TargetSelector = std::function<std::weak_ptr<ChatCharacter>(FVector, Field *)>;

    extern const TargetSelector PveTargetSelector;
}


class MonsterAi {
};


#endif //MONSTERAI_H
