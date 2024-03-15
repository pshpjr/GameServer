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
        Item(ObjectID id,ObjectManager& manager, GroupCommon& group, const FVector location, const float distance, char type)
            : GameObject(id,manager,group, location, {0, 0}, 0, eCharacterGroup::Item, type)
            , _range(location, distance)
        {
        }
        bool Collision(FVector point);

        void Take(ChatCharacter& target);

    private:
        CircleRange _range;
    };
}
