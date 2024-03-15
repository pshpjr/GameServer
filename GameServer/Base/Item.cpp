#include "Item.h"

#include "ObjectManager.h"
#include "../Data/TableData.h"


bool psh::Item::Collision(FVector point)
{
    return _range.Contains(point);
}

void psh::Item::Take(psh::ChatCharacter& target)
{
    _owner.DestroyActor(shared_from_this(),Location(),SEND_OFFSETS::BROADCAST,false,false);
}
