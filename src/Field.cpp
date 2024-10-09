﻿/*
 * Field.cpp
 *
 * 서버 내에서 필드(맵) 로직을 담당하는 클래스 구현
 *
 * 파일: Field.cpp
 * 설명: 게임 내 플레이어, 몬스터, 아이템 등 필드 상의 오브젝트 관리 및 처리
 * 작성자: [작성자명]
 * 날짜: [작성일]
 */

#include "Field.h"
#include <memory>
#include "Profiler.h"
#include "CLogger.h"
#include "DBData.h"
#include "DBThreadWrapper.h"
#include "GameObject.h"
#include "IVictimSelect.h"
#include "MonitorProtocol.h"
#include "Player.h"
#include "Server.h"
#include "TableData.h"
#include "Monster.h"
#include "MonsterSpawner.h"
#include "Rand.h"

// 필드 클래스 생성자 및 소멸자
psh::Field::Field(Server &server, const ServerInitData &data, const ServerType type, short mapSize, short sectorSize)
    : _dbThread{
        std::make_unique<DBThreadWrapper>(data.gameDBIP.c_str(), data.gameDBPort, data.gameDBID.c_str()
                                        , data.gameDBPwd.c_str(), "mydb")
    }
  , _server(server)
  , _initData(data)
  , _groupType(type)
  , _playerMap{std::make_unique<map_type>(mapSize, sectorSize)}
  , _monsterMap{std::make_unique<map_type>(mapSize, sectorSize)}
  , _itemMap{std::make_unique<map_type>(mapSize, sectorSize)}
  , _victimSelect{victim_select::GetVictimByServerType(type)}
  , _useMonitor{_initData.UseMonitorServer}
  , _nextDBSend(std::chrono::steady_clock::now())
  , _prevUpdate(std::chrono::steady_clock::now())
  , _logger{std::make_unique<CLogger>(std::format(L"field_{}", static_cast<int>(type)).c_str())}
  , _fieldSize(mapSize)
{
}

psh::Field::~Field() = default;

// 클라이언트가 필드에 입장할 때 호출
void psh::Field::OnEnter(SessionID id)
{
    auto dataPtr = _server.GetDbData(id);
    if (static_cast<ServerType>(dataPtr->ServerType()) != _groupType)
    {
        dataPtr->SetServerType(static_cast<char>(_groupType));

        const auto fieldSize = static_cast<float>(_fieldSize);
        dataPtr->SetLocation({RandomUtil::Rand(0.0, fieldSize), RandomUtil::Rand(0.0, fieldSize)});
    }

    // 플레이어 객체 생성 및 필드에 등록
    GameObjectData initData{dataPtr->Location(), {0, 0}, 200.0, eObjectType::Player, dataPtr->CharacterType()};
    auto playerPtr = std::make_shared<Player>(*this, initData, dataPtr, _dbThread.get());
    playerPtr->SetMap(nullptr);
    playerPtr->ObjectId(NextObjectId());

    // 플레이어 레벨 진입 패킷 생성 및 전송
    auto levelInfoPacket = SendBuffer::Alloc();
    MakeGame_ResLevelEnter(levelInfoPacket, playerPtr->AccountNumber(), playerPtr->ObjectId(), _groupType);
    SendPacket(playerPtr->SessionId(), levelInfoPacket);
    _players.insert({id, std::move(playerPtr)});

    _iocp->SetTimeout(id, 30000);
}

// 클라이언트가 필드에서 퇴장할 때 호출
void psh::Field::OnLeave(const SessionID id, int wsaErrCode)
{
    const auto it = _players.find(id);
    const PlayerRef playerPtr = it->second;

    if (playerPtr->Valid())
    {
        playerPtr->_data->SetLocation(playerPtr->Location());
        _dbThread->LeaveGroup(playerPtr->_data, &_server, id, GroupManager::BaseGroupID());
    }

    _players.erase(it);
    DestroyActor(playerPtr);
}

// 필드 내 상태 업데이트
void psh::Field::OnUpdate(const int milli)
{
    using namespace std::chrono;
    UpdateContent(milli);
    InsertWaitObjectInMap();
    CleanupDeleteWait();
    _fps++;

    if (_spawner)
    {
        _spawner->Update(milli);
    }

    if (steady_clock::now() < _nextDBSend)
    {
        return;
    }

    SendMonitor();
    _nextDBSend += 1s;
    _fps = 0;
}

// 클라이언트로부터 받은 패킷 처리
void psh::Field::OnRecv(const SessionID id, CRecvBuffer &recvBuffer)
{
    ePacketType type;
    recvBuffer >> type;
    auto &[_, playerPtr] = *_players.find(id);

    switch (type)
    {
        case eGame_ReqChangeComplete:
            RecvChangeComp(id, recvBuffer);
            break;
        case eGame_ReqMove:
            RecvMove(id, recvBuffer);
            break;
        case eGame_ReqAttack:
            RecvAttack(id, recvBuffer);
            break;
        case eGame_ReqLevelEnter:
            RecvReqLevelChange(id, recvBuffer);
            break;
        case eGame_ReqChat:
            RecvChat(id, recvBuffer);
            break;
        default:
            DebugBreak();
            break;
    }
}

// 특정 클라이언트가 떠날 때 이를 다른 플레이어에게 브로드캐스트
void psh::Field::BroadcastPlayerLeave(const PlayerRef &playerPtr)
{
    auto destroyBuf = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(destroyBuf, playerPtr->ObjectId(), false, static_cast<char>(removeResult::GroupChange));
    for (auto view = GetObjectView(ViewObjectType::Player, playerPtr->Location(), SEND_OFFSETS::BROADCAST);
         auto &player: view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(destroyBuf);
    }
}

// 클라이언트의 레벨 변경 요청을 처리
void psh::Field::RecvReqLevelChange(const SessionID id, CRecvBuffer &recvBuffer)
{
    AccountNo accountNo;
    ServerType type;
    GetGame_ReqLevelEnter(recvBuffer, accountNo, type);
    auto [_, playerPtr] = *_players.find(id);

    if (playerPtr->Valid())
    {
        auto obj = std::static_pointer_cast<GameObject>(playerPtr);
        DestroyActor(obj);
    }

    playerPtr->_data->SetLocation(playerPtr->Location());
    _dbThread->LeaveGroup(playerPtr->_data, &_server, id, _server.GetGroupId(type));
}

// 플레이어의 위치 변경 완료를 처리
void psh::Field::RecvChangeComp(const SessionID id, CRecvBuffer &recvBuffer)
{
    AccountNo accountNo;
    GetGame_ReqChangeComplete(recvBuffer, accountNo);
    auto &[_, playerPtr] = *_players.find(id);

    if (playerPtr->isDead())
    {
        playerPtr->Revive();
    }
    _dbThread->EnterGroup(playerPtr->_data);
    AddActor(std::static_pointer_cast<GameObject>(playerPtr));
}

// 플레이어의 채팅 메시지 처리
void psh::Field::RecvChat(const SessionID id, CRecvBuffer &recvByffer)
{
    auto &[_, player] = *_players.find(id);
    String chatData;
    GetGame_ReqChat(recvByffer, chatData);

    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvChat. Not Found Player. SessionID : %d", id);
        _iocp->DisconnectSession(id);
        return;
    }

    auto chatBuffer = SendBuffer::Alloc();
    MakeGame_ResChat(chatBuffer, player->ObjectId(), chatData);
    BroadcastToPlayer(player->Location(), {chatBuffer});
}

// 플레이어 이동 요청 처리
void psh::Field::RecvMove(const SessionID sessionId, CRecvBuffer &buffer)
{
    auto &[_, player] = *_players.find(sessionId);
    FVector location{};
    GetGame_ReqMove(buffer, location);

    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvMove. Not Found Player. SessionID : %d"
                     , sessionId);
        _iocp->DisconnectSession(sessionId);
        return;
    }

    if (location == player->Location())
    {
        auto moveBuffer = SendBuffer::Alloc();
        MakeGame_ResMove(moveBuffer, player->ObjectId(), player->ObjectType(), player->Location());
        SendPacket(player->SessionId(), moveBuffer);
        printf("InvalidLocation. objID : %d, AccountNo : %lld\n", player->ObjectType(), player->AccountNumber());
        return;
    }

    player->MoveStart(location);
}

// 플레이어 공격 요청 처리
void psh::Field::RecvAttack(const SessionID sessionId, CRecvBuffer &buffer)
{
    auto &[_, player] = *_players.find(sessionId);
    char type;
    FVector dir{};
    GetGame_ReqAttack(buffer, type, dir);

    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvAttack. Not Found Player. SessionID : %d"
                     , sessionId);
    }
    if (player->isDead())
    {
        return;
    }

    player->Attack(type, dir);
}

// 특정 위치의 플레이어에게 데이터를 브로드캐스트
void psh::Field::BroadcastToPlayer(FVector targetLocation, const std::vector<SendBuffer> &packets)
{
    for (auto view = GetObjectView(ViewObjectType::Player, targetLocation, SEND_OFFSETS::BROADCAST);
         const auto &player: view)
    {
        for (const SendBuffer &packet: packets)
        {
            std::static_pointer_cast<psh::Player>(player)->SendPacket(packet);
        }
    }
}

void psh::Field::SpawnItem(const shared<Item> &obj)
{
    ASSERT_CRASH(false, "Not implemented");
}

// 몬스터 생성 및 필드에 추가
void psh::Field::SpawnMonster(const shared<Monster> &obj)
{
    AddActor(std::static_pointer_cast<GameObject>(obj));
}

// 필드가 생성될 때 초기화
void psh::Field::OnCreate()
{
    String extraName = L"Group";
    extraName += std::to_wstring(static_cast<int>(static_cast<long>(GetGroupID())));
    _logger = std::make_unique<CLogger>(extraName.c_str());
}

// 공격 처리 로직
psh::victim_select::AttackResult psh::Field::ProcessAttack(AttackInfo info)
{
    return _victimSelect(*this, info);
}

// 특정 오브젝트 타입에 대한 시야 반환
psh::Field::view_type psh::Field::GetObjectView(ViewObjectType type, const FVector &location
                                              , std::span<const Sector> offsets)
{
    std::vector<map_type::SectorView> returnView;

    if ((type & ViewObjectType::Player) == ViewObjectType::Player)
    {
        returnView.push_back(_playerMap->GetSectorsFromOffset(location, offsets));
    }
    if ((type & ViewObjectType::Monster) == ViewObjectType::Monster)
    {
        returnView.push_back(_monsterMap->GetSectorsFromOffset(location, offsets));
    }
    if ((type & ViewObjectType::Item) == ViewObjectType::Item)
    {
        returnView.push_back(_itemMap->GetSectorsFromOffset(location, offsets));
    }

    return map_type::SectorView{returnView};
}

// 필드 내 특정 위치에서 시야 반환(구현되지 않음)
psh::Field::view_type psh::Field::GetObjectViewByPoint(ViewObjectType type, const std::list<FVector> &coordinate)
{
    throw std::logic_error("Not implemented");
}

// 오브젝트를 대기 목록에 추가
void psh::Field::AddActor(const shared<GameObject> &obj)
{
    _createWaits.push_back(obj);
}

// 오브젝트 삭제 대기 목록에 추가
void psh::Field::DestroyActor(shared<GameObject> &obj)
{
    obj->Valid(false);
    _delWaits.emplace_back(obj);
}

// 오브젝트 삭제 대기 목록에 추가 (rvalue 참조)
void psh::Field::DestroyActor(shared<GameObject> &&obj)
{
    DestroyActor(obj);
}

// 대기 중인 오브젝트를 필드에 삽입
void psh::Field::InsertWaitObjectInMap()
{
    while (!_createWaits.empty())
    {
        auto obj = _createWaits.front();
        _createWaits.pop_front();
        _objects.insert(obj);

        if (obj->ObjectType() != eObjectType::Player)
        {
            obj->ObjectId(NextObjectId());
        }

        const auto map = FindObjectMap(obj);
        map->Insert(obj->ObjectId(), obj, obj->Location());
        obj->Valid(true);
        obj->SetMap(map);

        obj->OnCreate();
    }
}

// 오브젝트 타입에 따라 적절한 맵 반환
psh::GameMap<psh::ObjectID, psh::shared<psh::GameObject> > *psh::Field::FindObjectMap(
    const shared<GameObject> &obj) const
{
    switch (obj->ObjectType())
    {
        case eObjectType::Player:
            return _playerMap.get();
        case eObjectType::Monster:
            return _monsterMap.get();
        case eObjectType::Item:
            return _itemMap.get();
        default:
            ASSERT_CRASH(false, L"InvalidType");
            return nullptr;
    }
}

// 삭제 대기 중인 오브젝트 정리
void psh::Field::CleanupDeleteWait()
{
    while (!_delWaits.empty())
    {
        auto obj = _delWaits.front();
        _delWaits.pop_front();
        obj->OnDestroy();

        if (const auto map = obj->GetMap();
            map != nullptr)
        {
            map->Delete(obj->ObjectId(), obj->Location());
        }
        _objects.erase(obj);
    }
}

// 다음 오브젝트 ID 반환
psh::ObjectID psh::Field::NextObjectId()
{
    return ++_objectId;
}

// 필드 내 오브젝트 업데이트
void psh::Field::UpdateContent(const int deltaMs)
{
    for (auto &obj: _objects)
    {
        obj->Update(deltaMs);
    }
}

// 모니터링 데이터 전송
void psh::Field::SendMonitor()
{
    printf("Players : %zd\nGroup : %ld, Work : %lld, Queue: %d, Handled : %lld\n",
           _players.size(), static_cast<long>(GetGroupID()), GetWorkTime(), GetQueued(), GetJobTps());

    if (!_useMonitor)
    {
        return;
    }

    auto [queued, enqueue, dequeue, delaySum] = _dbThread->GetMonitor();

    if (_monitorSession == InvalidSessionID())
    {
        auto client = _server.GetClientSession(_initData.MonitorServerIP, _initData.MonitorServerPort);
        if (client.HasError())
        {
            return;
        }
        _monitorSession = client.Value();
        _iocp->SetSessionStaticKey(_monitorSession, 0);
        SendLogin();
    }
    else if (!_server.IsValidSession(_monitorSession))
    {
        _monitorSession = InvalidSessionID();
        return;
    }

    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, static_cast<int>(1));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_WORK_TIME, static_cast<int>(GetWorkTime()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_JOB_QUEUE_SIZE, GetQueued());
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_JOB_TPS, static_cast<int>(GetJobTps()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SESSIONS, static_cast<int>(Sessions()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_PLAYERS, static_cast<int>(_players.size()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_TPS, static_cast<int>(dequeue));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_QUEUE_SIZE, static_cast<int>(queued));

    if (dequeue != 0)
    {
        SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_QUERY_AVG
                      , static_cast<int>(delaySum / static_cast<float>(dequeue)));
    }
}

// 모니터 로그인 전송
void psh::Field::SendLogin() const
{
    auto buffer = SendBuffer::Alloc();
    buffer << en_PACKET_SS_MONITOR_LOGIN << static_cast<WORD>(1) << static_cast<char>(static_cast<long>(GetGroupID()));
    _server.SendPacket(_monitorSession, buffer);
}

// 모니터링 데이터 전송
void psh::Field::SendMonitorData(const en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, const int value) const
{
    auto buffer = SendBuffer::Alloc();
    buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << static_cast<WORD>(1) << static_cast<char>(static_cast<long>(
        GetGroupID())) << type << value << static_cast<int>(time(nullptr));
    _server.SendPacket(_monitorSession, buffer);
}

// ViewObjectType 비트 연산자 오버로딩
psh::Field::ViewObjectType psh::operator|(Field::ViewObjectType lhs, Field::ViewObjectType rhs)
{
    using enumType = std::underlying_type_t<Field::ViewObjectType>;
    return static_cast<Field::ViewObjectType>(static_cast<enumType>(lhs) | static_cast<enumType>(rhs));
}

psh::Field::ViewObjectType psh::operator&(Field::ViewObjectType lhs, Field::ViewObjectType rhs)
{
    using enumType = std::underlying_type_t<Field::ViewObjectType>;
    return static_cast<Field::ViewObjectType>(static_cast<enumType>(lhs) & static_cast<enumType>(rhs));
}
