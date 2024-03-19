#pragma once
#include <memory>
#include <PacketGenerated.h>

#include "Range.h"

namespace psh
{
    class AttackManager;
    class ObjectManager;
    class GroupCommon;


    class GameObject : public enable_shared_from_this<GameObject>
    {
    public:
        GameObject(ObjectID id
                    , ObjectManager& owner
                    , GroupCommon& group
                   , FVector location
                   , FVector direction
                   , float moveSpeedPerSec
                   , eCharacterGroup characterType = eCharacterGroup::Object
                   , char type = 0);

        virtual ~GameObject();

        bool InRange(const Range& range) const;
        
        virtual void MakeCreatePacket(SendBuffer& buffer, bool spawn) const;
        
        void MoveStart(FVector destination);
        void MoveStop();
        virtual void Update(int delta);
        
        virtual void OnCreate() const
        {
        }

        virtual void OnDestroy() const
        {
        }
    
        
    public:
        [[nodiscard]] FVector OldLocation() const
        {
            return _oldLocation;
        }
        
        void OldLocation(FVector location)
        {
            _oldLocation = location;
        }

        void ObjectId(ObjectID id)
        {
            _objectId = id;
        }

        [[nodiscard]] ObjectID ObjectId() const
        {
            return _objectId;
        }

        [[nodiscard]] FVector Location() const
        {
            return _location;
        }

        void Location(FVector loc)
        {
            _location = loc;

        }

        [[nodiscard]] bool isMove() const
        {
            return _move;
        }

        [[nodiscard]] FVector Direction() const
        {
            return _direction;
        }
        [[nodiscard]] FVector Destination() const
        {
            return _destination;
        }

        [[nodiscard]] char Type() const
        {
            return _type;
        }
        
        [[nodiscard]] eCharacterGroup ObjectGroup() const
        {
            return _objectGroup;
        }
        psh::AttackManager* _attackManager = nullptr;//nullable

    private:
        ObjectID _objectId;
        FVector _location;
        FVector _direction;
        eCharacterGroup _objectGroup = eCharacterGroup::Object;
        char _type = 0;

    protected:
        virtual void OnUpdate(float delta)
        {
        }

        ObjectManager& _owner;
        GroupCommon& _group;

    private:
        float _moveSpeedPerSec;
        float MoveSpeedPerMs = _moveSpeedPerSec/1000.0f;
        FVector _oldLocation;
        FVector _destination;
        bool _move = false;
        bool _stateChanged = false;


    };
}
