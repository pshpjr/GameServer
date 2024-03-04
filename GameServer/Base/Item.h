#pragma once
#include "../Base/GameObject.h"
#include "Range.h"
#include "../Group/GroupCommon.h"

namespace psh
{
    class ChatCharacter;
}

namespace psh
{

    class Item : public GameObject
    {
    public:
        Item(ObjectID id, const FVector location, const float distance, char type)
            : GameObject(id, location, {0, 0}, 0, eCharacterGroup::Item, type), _range(location,distance)
        {
        }

        void Spawn();

        bool Collision(FVector point);

        void Take(shared_ptr<ChatCharacter>& target);

    private:
        CircleRange _range;
    };
}

