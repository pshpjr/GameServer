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

    //몬스터 어그로 대상이 되는 애를 선택하는 함수
    using TargetSelector = std::function<std::weak_ptr<GameObject>(TargetRequest, Field&)>;

    //Pvp냐 Pve냐에 따라 몬스터 선택 함수가 달라짐.
    TargetSelector GetTargetSelectorByType(ServerType type);
}


#endif //MONSTERAI_H
