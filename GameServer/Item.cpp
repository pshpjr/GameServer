#include "Item.h"

#include "ObjectManager.h"
#include "TableData.h"


bool psh::Item::Collision(FVector point)
{
    return _range.Contains(point);
}

void psh::Item::OnDestroy()
{
    _owner.RemoveFromMap(shared_from_this(),Location(),SEND_OFFSETS::BROADCAST,false,false, ObjectManager::removeResult::Die);
}

void psh::Item::Take(psh::ChatCharacter& target)
{
    _valid = false;
    _owner.DestroyActor(this);
}
