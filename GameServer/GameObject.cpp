#include "GameObject.h"

#include "GroupCommon.h"
#include "ObjectManager.h"
#include "TableData.h"

psh::GameObject::GameObject(ObjectID id
, ObjectManager& owner
, GroupCommon& group
                            , FVector location
                            , FVector direction
                            , float moveSpeedPerSec
                            , eCharacterGroup characterType
                            , char type):
                                            _location(location)
                                            , _direction(direction)
                                            , _objectGroup(characterType)
                                            , _type(type)
                                            , _owner(owner)
                                            ,_group(group)
                                            , _moveSpeedPerSec(moveSpeedPerSec)
                                            , _oldLocation(location)
                                            , _destination(location)
                                            , _objectId(id)
{
}

psh::GameObject::~GameObject()
{
    
};

void psh::GameObject::MakeCreatePacket(SendBuffer& buffer, bool spawn) const
{
    MakeGame_ResCreateActor(buffer, _objectId, _objectGroup, _type, _location, _direction,  spawn);

    if(isMove())
    {
        MakeGame_ResMove(buffer,_objectId,_objectGroup, _destination);
    }
}

bool psh::GameObject::InRange(const Range& range) const
{
    return range.Contains(_location);
}

void psh::GameObject::MoveStart(FVector destination)
{
    auto moveBuffer = SendBuffer::Alloc();
    MakeGame_ResMove(moveBuffer,_objectId,_objectGroup,destination);
    _group.SendInRange(_location,SEND_OFFSETS::BROADCAST,moveBuffer);

    if(destination == Location())
        return;
    
    _move = true;
    _destination = destination;
    _direction = (destination - Location()).Normalize();


}

void psh::GameObject::MoveStop()
{
    _move = false;
    auto moveStop = SendBuffer::Alloc();
    MakeGame_ResMoveStop(moveStop, ObjectId(), Location());
    _group.SendInRange(_location,SEND_OFFSETS::BROADCAST, moveStop);
}

void psh::GameObject::Update(int delta)
{
    if (_move)
    {
        const float DistanceToDestination = (_destination - _location).Size();
        const FVector moveDelta = _direction * delta * MoveSpeedPerMs;

        if (DistanceToDestination <= moveDelta.Size())
        {
            _owner.RequestMove(shared_from_this(), _destination);
            MoveStop();
        }
        else
        {
            _owner.RequestMove(shared_from_this(), _location + moveDelta);
        }
    }
    OnUpdate(delta);
}

