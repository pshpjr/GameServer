// GameObject.cpp
// 이 파일은 게임 객체 (GameObject) 클래스의 구현을 포함합니다.

#include "GameObject.h"
#include "Field.h"
#include "MoveComponent.h"
#include "Player.h"
#include "TableData.h"

// 전역 상수 및 전역 변수는 없음

namespace psh
{
    // GameObject 클래스 생성자
    GameObject::GameObject(Field& group, const GameObjectData& initData)
        : _field(group)
        , _location(initData.location)
        , _viewDirection(initData.direction)
        , _objectType(initData.objectType)
        , _templateId(initData.templateId) {}

    GameObject::~GameObject() = default;


    // 생성 패킷을 구성하는 함수
    void GameObject::MakeCreatePacket(SendBuffer& buffer, const bool spawn) const
    {
        MakeGame_ResCreateActor(buffer, _objectId, _objectType, _templateId, _location, _viewDirection, spawn);
    }


    // 업데이트 함수
    void GameObject::Update(int delta)
    {
        if (!Valid())
        {
            return;
        }

        OnUpdate(delta);
    }

    // 생성 시 호출되는 함수
    void GameObject::OnCreate()
    {
        auto createThisBuffer = SendBuffer::Alloc();
        MakeCreatePacket(createThisBuffer, true);

        _field.BroadcastToPlayer(Location(), {createThisBuffer});
        // 플레이어에게 생성 패킷 전송
        OnCreateImpl();
    }

    // 소멸 시 호출되는 함수
    void GameObject::OnDestroy()
    {
        auto destroyed = SendBuffer::Alloc();
        bool isDead = _removeReason == removeReason::Die;
        MakeGame_ResDestroyActor(destroyed, ObjectId(), isDead, static_cast<char>(_removeReason));

        _field.BroadcastToPlayer(Location(), {destroyed});
        // 플레이어에게 소멸 패킷 전송
        OnDestroyImpl();
    }

    void GameObject::OutFromMapWhenGroupMove()
    {
        //그룹 이동시 패킷을 받으면 안 된다.
        //이동 후 패킷을 받게 될 수 있음.
        //클라이언트 레벨 이동 후 패킷을 받으면 에러다.

        _map->Delete(ObjectId(), Location());
        _inMap = false;
        _map = nullptr;
    }

    void GameObject::IntoMap(GameMap<ObjectID, std::shared_ptr<GameObject>>* map)
    {
        map->Insert(ObjectId(), shared_from_this(), Location());
        _map = map;
        Valid(true);
    }

    std::ostream& operator<<(std::ostream& out, const GameObject& obj)
    {
        out << std::format("(GameObj: type:{}, Oid:{}, Loc:({},{})", static_cast<char>(obj.ObjectType())
                           , obj.ObjectId(), obj.Location().X, obj.Location().Y);
        return out;
    }
} // namespace psh
