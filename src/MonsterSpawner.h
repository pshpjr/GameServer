//
// Created by pshpj on 24. 10. 7.
//

#ifndef MONSTERSPAWNER_H
#define MONSTERSPAWNER_H
#include <ContentTypes.h>

#include "GameObject.h"

namespace psh
{
    class Field;

    /**
     *
     */
    class MonsterSpawner {
    public:
        MonsterSpawner(ServerType type, Field &field);

        void Update(int deltaTime);

    private:
        Field &_field;
        ServerType _type;
        const int MAX_MONSTERS = 30;

        GameObjectData _defaultData = {
            {0, 0}
          , {0, 0}
          , 200
          , eObjectType::Monster
          , 50
        };
    };
}


#endif //MONSTERSPAWNER_H
