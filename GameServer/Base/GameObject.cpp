#include "GameObject.h"

#include "DBConnection.h"
#include "../Group/GroupCommon.h"

psh::GameObject::GameObject(ObjectID id, FVector location, FVector direction, float moveSpeedPerSec,
    eCharacterGroup group, char type):
_owner(nullptr),
_map(nullptr),
_moveSpeedPerSec(moveSpeedPerSec), _oldLocation(location), _destination(location), _objectId(id),
_location(location), _direction(direction), _objectGroup(group), _type(type)
{
}

bool psh::GameObject::InSquareRange(const SquareRange& range) const
{
    return range.Contains(_location);
}

void psh::GameObject::GetInfo(SendBuffer& buffer, bool spawn) const
{
    MakeGame_ResCreateActor(buffer,_objectId,_objectGroup,_type,_location,_direction,_destination,_move,spawn);
}

bool psh::GameObject::inCircleRange(const CircleRange& range) const
{
    return range.Contains(_location);
}

void psh::GameObject::MoveStart(FVector destination)
{
    _move = true;
    _destination = destination;
    _direction = (destination - _location).Normalize();
}

void psh::GameObject::MoveStop()
{
    _move = false;
    auto moveStop =  SendBuffer::Alloc();
    MakeGame_ResMoveStop(moveStop, ObjectId(),Location());
    
    _owner->Broadcast(Location(),moveStop);
}

void psh::GameObject::Update(int delta)
{
    if(_move)
    {
        Move(delta);
    }
    OnUpdate(delta);
}

void psh::GameObject::SetGroup(GroupCommon* group)
{
    _owner = group;
}

void psh::GameObject::SetMap(GameMap<GameObject>* map)
{
    _map = map;
}

void psh::GameObject::Destroy(bool isDie)
{
    auto destroyPacket =  SendBuffer::Alloc();
    MakeGame_ResDestroyActor(destroyPacket, ObjectId(),isDie);
    _owner->Broadcast(Location(),destroyPacket);
    _owner->OnActorDestroy(shared_from_this());
}

void psh::GameObject::Move(float delta)
{
    const float DistanceToDestination = (_destination - _location).Size();
    if(DistanceToDestination < 10)
    {
        _location = _destination;
        OnMove();
        _oldLocation = _location;
        _move = false;
    }
    else
    {
        _oldLocation = _location;
        _location += (_direction * delta * MoveSpeedPerMs);
        _map->ClamToMap(_location);
        OnMove();
    }
}