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
    GameObject::GameObject(Field &group, const GameObjectData &initData)
        : _location(initData.location)
      , _viewDirection(initData.direction)
      , _objectType(initData.objectType)
      , _templateId(initData.templateId)
      , _field(group)
      , _oldLocation(initData.location)
      , _movementComponent{std::make_unique<MoveComponent>(*this, initData.moveSpeedPerSec / 1000.0f)}
    {
    }

    GameObject::~GameObject() = default;

    // 생성 패킷을 구성하는 함수
    void GameObject::MakeCreatePacket(SendBuffer &buffer, const bool spawn) const
    {
        MakeGame_ResCreateActor(buffer, _objectId, _objectType, _templateId, _location, _viewDirection, spawn);
        if (_movementComponent == nullptr)
        {
            return;
        }

        if (_movementComponent->IsMoving())
        {
            MakeGame_ResMove(buffer, _objectId, _objectType, _movementComponent->Destination());
        }
    }

    // 이동 시작 함수
    void GameObject::MoveStart(const FVector destination) const
    {
        _movementComponent->MoveStart(destination);
    }

    // 이동 멈춤 함수
    void GameObject::MoveStop() const
    {
        _movementComponent->MoveStop();
    }

    bool GameObject::IsMoving() const
    {
        return _movementComponent->IsMoving();
    }

    // 업데이트 함수
    void GameObject::Update(const int delta)
    {
        if (!Valid())
        {
            return;
        }

        if (_movementComponent != nullptr)
        {
            _movementComponent->Update(delta);
        }
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
        MakeGame_ResDestroyActor(destroyed, ObjectId(), true, static_cast<char>(_removeReason));

        _field.BroadcastToPlayer(Location(), {destroyed});
        // 플레이어에게 소멸 패킷 전송
        OnDestroyImpl();
    }

    std::ostream &operator<<(std::ostream &out, const GameObject &obj)
    {
        out << std::format("(GameObj: type:{}, Oid:{}, Loc:({},{})", static_cast<char>(obj.ObjectType())
                         , obj.ObjectId(), obj.Location().X, obj.Location().Y);
        return out;
    }
} // namespace psh
