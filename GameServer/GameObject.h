#pragma once
#include <memory>
#include <PacketGenerated.h>
#include "Sector.h"
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
        
        virtual void OnCreate()
        {
        }


        //삭제해야 할 경우 DestroyActor로 요청만 하고
        //맵에서 지우는 등 각종 정리 작업은 여기서
        //objManager에 Cleanup에서 맵에 지우라고 해도 되는데 왜 이 함수가 필요하냐?
        //왜 삭제되는지는 자기만 알아서. 
        virtual void OnDestroy()
        {
            
        }
    


        // struct MoveDebug
        // {
        //     psh::Sector before;
        //     psh::Sector after;
        // };
        // static const int size = 1024;
        // MoveDebug debug[size];
        // int index = 0;

        // void WriteMoveLog(Sector before, Sector after)
        // {
        //     debug[(index++) & size] = { before,after };
        // }
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
        [[nodiscard]] bool InMap() const {return _inMap;}
        void InMap(bool inMap) { _inMap = inMap;}
        
        psh::AttackManager* _attackManager = nullptr;//nullable

        void SetNeedUpdate(bool needUpdate){_needUpdate = needUpdate;}
        [[nodiscard]] bool NeedUpdate() const {return _needUpdate;}
    private:
        ObjectID _objectId;
        FVector _location;
        FVector _direction;
        eCharacterGroup _objectGroup = eCharacterGroup::Object;
        char _type = 0;

    protected:
        virtual void OnUpdate(int delta)
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
        bool _needUpdate = false;
        bool _inMap = false;
    };
}
