#include "GameObject.h"

#include "Field.h"
#include "Player.h"
#include "TableData.h"


psh::Sector TableIndexFromDiff(const psh::Sector sectorDiff)
{
    return psh::Sector(sectorDiff.x + 1, sectorDiff.y + 1);
}

psh::GameObject::GameObject(Field &group, const GameObjectData &initData):
                                                                         _location(initData.location)
                                                                       , _direction(initData.direction)
                                                                       , _objectType(initData.objectType)
                                                                       , _objectIndex(initData.templateId)
                                                                       , _field(group)
                                                                       , _moveSpeedPerSec(initData.moveSpeedPerSec)
                                                                       , _oldLocation(initData.location)
                                                                       , _destination(initData.location)
{
}

psh::GameObject::~GameObject() = default;

void psh::GameObject::MakeCreatePacket(SendBuffer &buffer, const bool spawn) const
{
    MakeGame_ResCreateActor(buffer, _objectId, _objectType, _objectIndex, _location, _direction, spawn);

    if (isMove())
    {
        MakeGame_ResMove(buffer, _objectId, _objectType, _destination);
    }
}

void psh::GameObject::MoveStart(const FVector destination)
{
    auto moveBuffer = SendBuffer::Alloc();
    MakeGame_ResMove(moveBuffer, _objectId, _objectType, destination);

    for (auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
         const auto &player: view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(moveBuffer);
    }

    _move = true;
    _destination = destination;
    _direction = (destination - Location()).Normalize();

    ASSERT_CRASH(!isnan(_direction.X), "InvalidDestination");
}

void psh::GameObject::MoveStop()
{
    _move = false;
    auto moveStop = SendBuffer::Alloc();
    MakeGame_ResMoveStop(moveStop, ObjectId(), Location());


    for (auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
         const auto& player: view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(moveStop);
    }
}

void psh::GameObject::Update(const int delta)
{
    if (!Valid())
    {
        return;
    }

    HandleMove(delta);
    OnUpdate(delta);
}

void psh::GameObject::OnCreate()
{
    auto createThisBuffer = SendBuffer::Alloc();
    MakeCreatePacket(createThisBuffer, true);

    for (auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
         const auto &player: view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(createThisBuffer);
    }
    OnCreateImpl();
}

void psh::GameObject::OnDestroy()
{
    auto buf = SendBuffer::Alloc();

    MakeGame_ResDestroyActor(buf, ObjectId(), true, _removeReason);

    auto view = _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST);
    for (const auto &player: _field.GetPlayerView(Location(), SEND_OFFSETS::BROADCAST))
    {
        std::static_pointer_cast<Player>(player)->SendPacket(buf);
    }

    OnDestroyImpl();
}

void psh::GameObject::BroadcastSectorChange(const int sectorIndex) const
{
    auto deletePlayers = _field.GetPlayerView(OldLocation(), SEND_OFFSETS::DeleteTable[sectorIndex]);
    auto newPlayers = _field.GetPlayerView(Location(), SEND_OFFSETS::CreateTable[sectorIndex]);
    {
        auto deleteThisBuffer = SendBuffer::Alloc();
        MakeGame_ResDestroyActor(deleteThisBuffer, ObjectId(), false, Move);
        for (auto &player: deletePlayers)
        {
            std::static_pointer_cast<Player>(player)->SendPacket(deleteThisBuffer);
        }


        auto createThisBuffer = SendBuffer::Alloc();
        MakeCreatePacket(createThisBuffer, false);
        for (auto &player: newPlayers)
        {
            std::static_pointer_cast<Player>(player)->SendPacket(createThisBuffer);
        }
    }
}

void psh::GameObject::HandleMove(const int delta)
{
    if (!_move)
    {
        return;
    }

    UpdateLocation(delta);

    const auto oldSector = _map->GetSector(OldLocation());
    const auto newSector = _map->GetSector(Location());

    if (oldSector == newSector)
    {
        return;
    }

    _map->Insert(ObjectId(), shared_from_this(), Location());
    _map->Delete(ObjectId(), OldLocation());

    const auto [x, y] = Clamp(newSector - oldSector, -1, 1);
    const auto sectorIndex = SEND_OFFSETS::getDirectionIndex(x, y);

    BroadcastSectorChange(sectorIndex);

    if (ObjectType() != eObjectType::Player)
    {
        return;
    }

    auto delObjects = _field.GetObjectView(OldLocation(), SEND_OFFSETS::DeleteTable[sectorIndex]);
    auto deleteObjectsBuffer = SendBuffer::Alloc();
    for (auto &obj: delObjects)
    {
        MakeGame_ResDestroyActor(deleteObjectsBuffer, obj->ObjectId(), false, Move);
    }
    if (deleteObjectsBuffer.Size() != 0)
    {
        static_cast<Player*>(this)->SendPacket(deleteObjectsBuffer);
    }


    auto createObjects = _field.GetObjectView(Location(), SEND_OFFSETS::CreateTable[sectorIndex]);
    auto createObjectsBuffer = SendBuffer::Alloc();

    for (auto &obj: createObjects)
    {
        obj->MakeCreatePacket(createObjectsBuffer, false);
    }

    if (createObjectsBuffer.Size() != 0)
    {
        static_cast<Player *>(this)->SendPacket(createObjectsBuffer);
    }
}

void psh::GameObject::UpdateLocation(const int delta)
{
    const float DistanceToDestination = (_destination - _location).Size();

    if (const FVector moveDelta = _direction * static_cast<float>(delta) * MoveSpeedPerMs;
        DistanceToDestination <= moveDelta.Size())
    {
        _map->ClampToMap(_destination);
        OldLocation(Location());
        Location(_destination);
        MoveStop();
    }
    else
    {
        auto destination = _location + moveDelta;
        _map->ClampToMap(destination);
        OldLocation(Location());
        Location(destination);
    }
}
