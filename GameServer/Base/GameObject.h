#pragma once
#include <memory>
#include <PacketGenerated.h>

#include "Range.h"

namespace psh
{
    class GameMap;
}

namespace psh
{
    struct Range;

    class GameObject 
    {
    public:
        GameObject(ObjectID id,FVector location, FVector direction, float moveSpeedPerSec,eCharacterGroup group = eCharacterGroup::Object, char type = 0):
            _moveSpeedPerSec(moveSpeedPerSec), _oldLocation(location), _destination(location), _objectId(id),
            _location(location), _direction(direction), _characterGroup(group), _type(type)
        {
        }
        
        virtual ~GameObject() = default;
        
        bool inSquareRange(const SquareRange& range) const
        {
            return range.Contains(_location);
        }
        
        virtual void GetInfo(SendBuffer& buffer, bool spawn) const
        {
            MakeGame_ResCreateActor(buffer,_objectId,_characterGroup,_type,_location,_direction,_destination,_move,spawn);
        }
        bool inCircleRange(const CircleRange& range) const
        {
            return range.Contains(_location);
        }

        virtual void MoveStart(FVector destination)
        {
            _move = true;
            _destination = destination;
            _direction = (destination - _location).Normalize();
        }
        void Update(int delta)
        {
            if(_move)
            {
                Move(delta);
            }
            OnUpdate(delta);
        }
    
    protected:
        virtual void OnUpdate(float delta){}
        virtual void OnMove(){}
        Group* _owner;

    public:
        void SetGroup(Group* group)
        {
            _owner = group;
        }

    private:
        void Move(float delta)
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
                OnMove();
            }
        }
        
        float _moveSpeedPerSec;
        float MoveSpeedPerMs = _moveSpeedPerSec/1000.0f;
        FVector _oldLocation;
        FVector _destination;
        bool _move = false;
        bool _stateChanged = false;
        
        ObjectID _objectId;

    public:
        [[nodiscard]] FVector OldLocation() const
        {
            return _oldLocation;
        }

        [[nodiscard]] ObjectID ObjectId() const
        {
            return _objectId;
        }

        [[nodiscard]] FVector Location() const
        {
            return _location;
        }
        [[nodiscard]] bool isMove() const
        {
            return _move;
        }
        [[nodiscard]] FVector Direction()const
        {
            return _direction;
        }

    private:
        FVector _location;
        FVector _direction;
        eCharacterGroup _characterGroup = eCharacterGroup::Object;
        char _type = 0;
    };


}


