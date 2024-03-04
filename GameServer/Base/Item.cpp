#include "Item.h"
#include "Group.h"
void psh::Item::Spawn()
{
    auto spawnBuffer = SendBuffer::Alloc();

    GetInfo(spawnBuffer,true);
            
    _owner->Broadcast(Location(),spawnBuffer);
}

bool psh::Item::Collision(FVector point)
{
    return _range.Contains(point);
}

void psh::Item::Take(shared_ptr<ChatCharacter>& target)
{
    
}
