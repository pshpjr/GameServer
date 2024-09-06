#pragma once
#include <format>
#include <memory>
#include <PacketGenerated.h>
#include "ContentTypes.h"

namespace psh
{
    class AttackManager;
    class Field;

    enum removeResult : char
    {
        GroupChange, Move, Die
    };

    template <typename T>
    class GameMap;
    class GameObject;

    //0~25 : 플레이어 50~75: 몬스터. 100~ 아이템
    using TemplateID = char;
    struct GameObjectData
    {
        FVector location{};
        FVector direction{};

        //templateID 같은 이름으로 뺄 수 있을듯.
        float moveSpeedPerSec{};
        eObjectType objectType = eObjectType::Object;
        TemplateID templateId = 127;
    };

    //오브젝트 소멸 조건 되면 자기가 죽었다고 주변에 알아서 알리기.
    class GameObject : public std::enable_shared_from_this<GameObject>
    {
    public:
        GameObject(Field& group, const GameObjectData& initData);

        virtual ~GameObject();
        virtual void MakeCreatePacket(SendBuffer& buffer, bool spawn) const;

        void MoveStart(FVector destination);
        void MoveStop();
        virtual void Update(int delta);

        //생성, 소멸시 주변에 알리는 것은 field에서 한다.
        virtual void OnCreate()
        {
        }

        //생성, 소멸시 주변에 알리는 것은 field에서 한다.
        //사망 후 아이템 드롭 등을 요청만 함.
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
        friend std::ostream& operator<<(std::ostream& out, const GameObject& obj)
        {
            out << std::format("(GameObj: type:{}, Oid:{}, Loc:({},{})", static_cast<char>(obj.ObjectType())
                , obj.ObjectId(), obj.Location().X, obj.Location().Y);

            return out;
        }

        [[nodiscard]] FVector OldLocation() const
        {
            return _oldLocation;
        }

        void OldLocation(const FVector location)
        {
            _oldLocation = location;
        }

        void ObjectId(const ObjectID id)
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

        void Location(const FVector loc)
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

        [[nodiscard]] char objectIndex() const
        {
            return _objectIndex;
        }

        [[nodiscard]] eObjectType ObjectType() const
        {
            return _objectType;
        }

        [[nodiscard]] bool Valid() const
        {
            return _inMap;
        }

        void Valid(const bool inMap)
        {
            _inMap = inMap;
        }

        GameMap<shared<GameObject>>* GetMap() const
        {
            return _map;
        }

        void SetMap(GameMap<shared<GameObject>>* map)
        {
            _map = map;
        }

        [[nodiscard]] Field& GetField() const
        {
            return _field;
        }

    private:
        void HandleMove(int delta);

        void UpdateLocation(int delta);

        void BroadcastSectorChange(int sectorIndex) const;

        ObjectID _objectId{};
        FVector _location{};
        FVector _direction{};
        eObjectType _objectType = eObjectType::Object;
        char _objectIndex = 0;

    protected:
        virtual void OnUpdate(int delta)
        {
        }

        Field& _field;
        GameMap<shared<GameObject>>* _map{nullptr};

    private:
        float _moveSpeedPerSec;
        float MoveSpeedPerMs = _moveSpeedPerSec / 1000.0f;
        FVector _oldLocation;
        FVector _destination;
        bool _move = false;
        bool _inMap = false;
    };
}
