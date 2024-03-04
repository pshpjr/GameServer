#pragma once
#include <memory>
#include <PacketGenerated.h>

#include "Range.h"

namespace psh
{
    class GroupCommon;
    template <typename T>
    class GameMap;
    
    class GameObject : public enable_shared_from_this<GameObject>
    {
    public:
        GameObject(ObjectID id, FVector location, FVector direction, float moveSpeedPerSec,
                   eCharacterGroup group = eCharacterGroup::Object, char type = 0);

        virtual ~GameObject() = default;
        
        bool InSquareRange(const SquareRange& range) const;

        virtual void GetInfo(SendBuffer& buffer, bool spawn) const;

        bool inCircleRange(const CircleRange& range) const;

        virtual void MoveStart(FVector destination);

        void Update(int delta);
    public:
        [[nodiscard]] FVector OldLocation() const
        {
            return _oldLocation;
        }

        void  ObjectId(ObjectID id) 
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
            _location =  loc;
            _oldLocation = loc;
            _destination = loc;
        }
        [[nodiscard]] bool isMove() const
        {
            return _move;
        }
        [[nodiscard]] FVector Direction()const
        {
            return _direction;
        }

        [[nodiscard]] eCharacterGroup ObjectGroup() const
        {
            return _objectGroup;
        }
        [[nodiscard]] GameMap<GameObject>* Map() const
        {
            return _map;
        }
    private:
        FVector _location;
        FVector _direction;
        eCharacterGroup _objectGroup = eCharacterGroup::Object;
        char _type = 0;
    
    
    protected:
        virtual void OnUpdate(float delta){}
        virtual void OnMove(){}
        GroupCommon* _owner;
        GameMap<GameObject>* _map;
        
    public:
        void SetGroup(GroupCommon* group);

        void SetMap(GameMap<GameObject>* map);
        void Destroy(bool isDie);

    private:

        
        void Move(float delta);

        
        float _moveSpeedPerSec;
        float MoveSpeedPerMs = _moveSpeedPerSec/1000.0f;
        FVector _oldLocation;
        FVector _destination;
        bool _move = false;
        bool _stateChanged = false;
        
        ObjectID _objectId;
    };

}


