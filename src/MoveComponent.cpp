#include "MoveComponent.h"
#include "GameObject.h"
#include "Field.h"
#include "TableData.h"

namespace psh
{
    MoveComponent::MoveComponent(GameObject& owner, float baseSpeed)
        : _owner(owner)
        , _destination{owner.Location()}
        , _baseSpeedPerMs(baseSpeed)
        , _moveSpeedPerMs{baseSpeed}
        , _oldLocation{owner.Location()} {}

    void MoveComponent::Update(int delta)
    {
        if (!_isMoving)
        {
            return;
        }

        auto& map = *_owner.GetMap();
        auto& field = _owner.GetField();

        // 위치 업데이트 수행
        UpdateLocation(delta);
        _owner.ViewDirection(_moveDirection);
        // 섹터가 변경되었는지 확인 후 처리
        const auto oldLoc = _oldLocation;
        const auto curLoc = _owner.Location();

        const auto oldSec = map.GetSectorAtLocation(oldLoc);
        const auto newSec = map.GetSectorAtLocation(curLoc);
        if (oldSec == newSec)
        {
            return;
        }

        const auto objId = _owner.ObjectId();

        // 섹터가 변경되면 객체의 섹터 정보 갱신 및 맵에 재삽입
        map.Insert(objId, _owner.shared_from_this(), curLoc);
        map.Delete(objId, oldLoc);

        // 섹터 변경 방향 계산
        const auto deltaSector = Clamp(newSec - oldSec, -1, 1);

        // 섹터 변경 브로드캐스트
        BroadcastSectorChange(deltaSector);

        //플레이어면 추가 정보 받아오기
        if (_owner.ObjectType() != eObjectType::Player)
        {
            return;
        }

        RefreshSectorObjects(field, deltaSector);
    }

    void MoveComponent::MoveStart(const FVector& destination)
    {
        _destination = destination;
        _moveDirection = (_destination - _owner.Location()).Normalize();
        _isMoving = true;

        // 이동 시작 패킷 생성 및 전송
        auto moveBuffer = SendBuffer::Alloc();
        MakeGame_ResMove(moveBuffer, _owner.ObjectId(), _owner.ObjectType(), _destination);
        _owner.GetField().BroadcastToPlayer(_owner.Location(), {moveBuffer});

        ASSERT_CRASH(!isnan(_moveDirection.X), "InvalidDestination");
    }

    void MoveComponent::MoveStop()
    {
        if (_isMoving == false)
        {
            return;
        }

        _isMoving = false;

        // 이동 멈춤 패킷 생성 및 전송
        auto moveStop = SendBuffer::Alloc();
        MakeGame_ResMoveStop(moveStop, _owner.ObjectId(), _owner.Location());
        _owner.GetField().BroadcastToPlayer(_owner.Location(), {moveStop});
    }

    void MoveComponent::UpdateLocation(int delta)
    {
        const float distanceToDestination = (_destination - _owner.Location()).Size();

        if (isnan(distanceToDestination))
        {
            MoveStop();
            return;
        }

        if (const FVector moveDelta = _moveDirection * static_cast<float>(delta) * _moveSpeedPerMs;
            distanceToDestination <= moveDelta.Size())
        {
            _oldLocation = _owner.Location(); // 기존 위치 저장
            _owner.Location(_destination); // 새로운 위치 설정
            MoveStop(); // 목적지 도달 시 이동 멈춤
        }
        else
        {
            FVector newLocation = _owner.Location() + moveDelta;
            _oldLocation = _owner.Location(); // 기존 위치 저장
            _owner.Location(newLocation); // 새로운 위치 설정
        }
    }

    void MoveComponent::BroadcastSectorChange(Sector delta) const
    {
        const auto sectorIndex = SEND_OFFSETS::getDirectionIndex(delta);

        auto deletePlayers = _owner.GetField()
                                   .GetObjectView(Field::ViewObjectType::Player, _oldLocation,
                                                  SEND_OFFSETS::DeleteTable[sectorIndex]);
        auto newPlayers = _owner.GetField().GetObjectView(Field::ViewObjectType::Player, _owner.Location(),
                                                          SEND_OFFSETS::CreateTable[sectorIndex]);

        // 기존 플레이어에게 삭제 패킷 전송
        auto deleteThisBuffer = SendBuffer::Alloc();
        MakeGame_ResDestroyActor(deleteThisBuffer, _owner.ObjectId(), false, static_cast<char>(removeReason::Move));
        for (auto& player : deletePlayers)
        {
            std::static_pointer_cast<Player>(player)->SendPacket(deleteThisBuffer);
        }

        // 새로운 플레이어에게 생성 패킷 전송
        auto createThisBuffer = SendBuffer::Alloc();
        _owner.MakeCreatePacket(createThisBuffer, false);
        for (auto& player : newPlayers)
        {
            std::static_pointer_cast<Player>(player)->SendPacket(createThisBuffer);
        }
    }

    void MoveComponent::RefreshSectorObjects(Field& field
                                             , Sector delta) const
    {
        const auto sectorIndex = SEND_OFFSETS::getDirectionIndex(delta);

        auto delObjects =
            field.GetObjectView(Field::ViewObjectType::All, _oldLocation
                                , SEND_OFFSETS::DeleteTable[sectorIndex]);
        for (const auto& obj : delObjects)
        {
            auto deleteObjectsBuffer = SendBuffer::Alloc();
            MakeGame_ResDestroyActor(deleteObjectsBuffer, obj->ObjectId(), false
                                     , static_cast<char>(removeReason::Move));
            static_cast<Player*>(&_owner)->SendPacket(deleteObjectsBuffer);
        }

        auto createObjects =
            field.GetObjectView(Field::ViewObjectType::All, _owner.Location()
                                , SEND_OFFSETS::CreateTable[sectorIndex]);

        for (const auto& obj : createObjects)
        {
            auto createObjectsBuffer = SendBuffer::Alloc();
            obj->MakeCreatePacket(createObjectsBuffer, false);
            static_cast<Player*>(&_owner)->SendPacket(createObjectsBuffer);
        }
    }


    [[nodiscard]] bool MoveComponent::IsMoving() const
    {
        return _isMoving;
    }

    [[nodiscard]] FVector MoveComponent::Destination() const
    {
        return _destination;
    }
}
