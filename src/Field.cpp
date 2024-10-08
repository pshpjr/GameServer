#include "Field.h"

#include <Memory>

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


psh::Field::Field(Server &server
                , const ServerInitData &data
                , const ServerType type
                , short mapSize
                , short sectorSize)
    : _dbThread{
        std::make_unique<DBThreadWrapper>(
                                          data.gameDBIP.c_str()
                                        , data.gameDBPort
                                        , data.gameDBID.c_str()
                                        , data.gameDBPwd.c_str()
                                        , "mydb")
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
    if (_groupType == ServerType::Easy)
        _spawner = std::make_unique<MonsterSpawner>(ServerType::Easy, *this);
}

psh::Field::~Field() = default;

void psh::Field::OnEnter(SessionID id)
{
    auto dataPtr = _server.GetDbData(id);

    //접속한 유저의 정보 받아옴. 다른 곳에서 왔다면 무작위 위치 지정.
    if (static_cast<ServerType>(dataPtr->ServerType()) != _groupType)
    {
        dataPtr->SetServerType(static_cast<char>(_groupType));
        dataPtr->SetLocation(FVector(std::rand() % _fieldSize, std::rand() % _fieldSize));
    }

    //플레이어 객체 생성 리스트에 추가. 맵에는 클라에서 이동 완료 패킷 받았을 때
    GameObjectData initData{
        dataPtr->Location()
      , {0, 0}
      , 200.0
      , eObjectType::Player
      , dataPtr->CharacterType()
    };

    auto playerPtr = std::make_shared<Player>(*this, initData,
                                              dataPtr, _dbThread.get());
    playerPtr->SetMap(nullptr);
    playerPtr->ObjectId(NextObjectId());

    auto levelInfoPacket = SendBuffer::Alloc();
    MakeGame_ResLevelEnter(levelInfoPacket, playerPtr->AccountNumber(), playerPtr->ObjectId(), _groupType);
    SendPacket(playerPtr->SessionId(), levelInfoPacket);

    _players.insert({id, std::move(playerPtr)});

    _iocp->SetTimeout(id, 30000);
}

void psh::Field::OnLeave(const SessionID id, int wsaErrCode)
{
    const auto it = _players.find(id);
    const PlayerRef playerPtr = it->second;

    //살아있는 상태로 종료/필드 이동
    if (playerPtr->Valid())
    {
        playerPtr->_data->SetLocation(playerPtr->Location());

        _dbThread->LeaveGroup(playerPtr->_data, &_server, id, GroupManager::BaseGroupID());
    }

    _players.erase(it);
    DestroyActor(playerPtr);
}


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

void psh::Field::OnRecv(const SessionID id, CRecvBuffer &recvBuffer)
{
    ePacketType type;
    recvBuffer >> type;
    auto &[_,playerPtr] = *_players.find(id);
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

void psh::Field::BroadcastPlayerLeave(const PlayerRef &playerPtr)
{
    auto destroyBuf = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(destroyBuf, playerPtr->ObjectId(), false, GroupChange);
    for (auto view = GetPlayerView(playerPtr->Location(), SEND_OFFSETS::BROADCAST);
         auto &player: view)
    {
        std::static_pointer_cast<Player>(player)->SendPacket(destroyBuf);
    }
}

void psh::Field::RecvReqLevelChange(const SessionID id, CRecvBuffer &recvBuffer)
{
    AccountNo accountNo;
    ServerType type;
    GetGame_ReqLevelEnter(recvBuffer, accountNo, type);

    auto [_, playerPtr] = *_players.find(id);

    if (playerPtr->Valid())
    {
        auto obj = static_pointer_cast<GameObject>(playerPtr);
        DestroyActor(obj);
    }

    playerPtr->_data->SetLocation(playerPtr->Location());

    _dbThread->LeaveGroup(playerPtr->_data, &_server, id, _server.GetGroupId(type));
}

void psh::Field::RecvChangeComp(const SessionID id, CRecvBuffer &recvBuffer)
{
    AccountNo accountNo;
    GetGame_ReqChangeComplete(recvBuffer, accountNo);

    auto &[_,playerPtr] = *_players.find(id);

    if (playerPtr->isDead())
    {
        playerPtr->Revive();
    }

    _dbThread->EnterGroup(playerPtr->_data);
    AddActor(static_pointer_cast<GameObject>(playerPtr));
}

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
    for (const auto view = GetPlayerView(player->Location(), SEND_OFFSETS::BROADCAST);
         auto &p: view)
    {
        std::static_pointer_cast<Player>(p)->SendPacket(chatBuffer);
    }
}

void psh::Field::RecvMove(const SessionID sessionId, CRecvBuffer &buffer)
{
    auto &[_,player] = *_players.find(sessionId);
    FVector location{};
    GetGame_ReqMove(buffer, location);

    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvMove. Not Found Player. SessionID : %d"
                     , sessionId);
        _iocp->DisconnectSession(sessionId);
        return;
    }

    //자기가 있는 위치로 이동해서 nan되는 문제 발생시
    //단순히 리턴 하면 더미에서 move 패킷 검증올 못함.
    //그거 방지용
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

void psh::Field::RecvAttack(const SessionID sessionId, CRecvBuffer &buffer)
{
    auto &[_,player] = *_players.find(sessionId);
    char type;
    FVector dir{};
    GetGame_ReqAttack(buffer, type, dir);
    std::cout << dir;
    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvAttack. Not Found Player. SessionID : %d"
                     , sessionId);
        _iocp->DisconnectSession(sessionId);
    }
    if (player->isDead())
    {
        //printf("ResAttack playerDead, Account : %d\n", player->AccountNumber() );
        return;
    }

    player->Attack(type, dir);
}

void psh::Field::BroadcastToPlayer(FVector targetLocation, const std::vector<SendBuffer> &packets)
{
    for (const auto view = GetPlayerView(targetLocation, SEND_OFFSETS::BROADCAST);
         auto &player: view)
    {
        for (const SendBuffer &packet: packets)
        {
            std::static_pointer_cast<psh::Player>(player)->SendPacket(packet);
        }
    }
}

void psh::Field::SpawnMonster(const shared<Monster> &obj)
{
    AddActor(std::static_pointer_cast<GameObject>(obj));
}

psh::FVector psh::Field::GetRandomLocation()
{
    return {
        static_cast<float>(RandomUtil::Rand(0, _fieldSize))
      , static_cast<float>(RandomUtil::Rand(0, _fieldSize))
    };
}

size_t psh::Field::GetMonsterCount()
{
    return _monsterMap->Objects();
}

void psh::Field::OnCreate()
{
    String extraName = L"Group";
    extraName += std::to_wstring(static_cast<int>(static_cast<long>(GetGroupID())));
    _logger = std::make_unique<CLogger>(extraName.c_str());
}

psh::victim_select::AttackResult psh::Field::ProcessAttack(AttackInfo info)
{
    return _victimSelect(*this, info);
}


void psh::Field::AddActor(const shared<GameObject> &obj)
{
    _createWaits.push_back(obj);
}

void psh::Field::DestroyActor(shared<GameObject> &obj)
{
    obj->Valid(false);

    _delWaits.emplace_back(obj);
}

void psh::Field::DestroyActor(shared<GameObject> &&obj)
{
    DestroyActor(obj);
}


void psh::Field::InsertWaitObjectInMap()
{
    while (!_createWaits.empty())
    {
        auto obj = _createWaits.front();
        _createWaits.pop_front();

        _objects.insert(obj);

        const auto map = FindObjectMap(obj);
        map->Insert(obj, obj->Location());
        obj->Valid(true);
        obj->SetMap(map);

        //플레이어의 경우 요청하기 전에 미리 설정함.
        //좋은 코드인가
        if (obj->ObjectType() != eObjectType::Player)
        {
            obj->ObjectId(NextObjectId());
        }


        obj->OnCreate();
    }
}

psh::GameMap<psh::shared<psh::GameObject> > *psh::Field::FindObjectMap(const shared<GameObject> &obj) const
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
            map->Delete(obj, obj->Location());
        }

        _objects.erase(obj);
    }
}

psh::ObjectID psh::Field::NextObjectId()
{
    return ++_objectId;
}

void psh::Field::UpdateContent(const int deltaMs)
{
    for (auto &obj: _objects)
    {
        obj->Update(deltaMs);
    }
}

void psh::Field::SendMonitor()
{
    printf("Players : %zd\n"
           "Group : %ld, Work : %lld, Queue: %d, Handled : %lld\n"

         , _players.size()
         , static_cast<long>(GetGroupID()), GetWorkTime(), GetQueued(), GetJobTps()
          );

    if (!_useMonitor)
    {
        return;
    }

    auto [queued, enqueue, dequeue, delaySum] = _dbThread->GetMonitor();
    //printf("DBQueued: %lld, DBEnqueue : %lld, DBDequeue : %lld, DBDelayAvg : %f\n"
    //    , monitor.queued, monitor.enqueue, monitor.dequeue, monitor.delaySum / float(monitor.dequeue));

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
    else if (_server.IsValidSession(_monitorSession) == false)
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

void psh::Field::SendLogin() const
{
    auto buffer = SendBuffer::Alloc();

    buffer << en_PACKET_SS_MONITOR_LOGIN << static_cast<WORD>(1) << static_cast<char>(static_cast<long>(GetGroupID()));
    _server.SendPacket(_monitorSession, buffer);
}

void psh::Field::SendMonitorData(const en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, const int value) const
{
    auto buffer = SendBuffer::Alloc();

    buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << static_cast<WORD>(1) << static_cast<char>(static_cast<long>(
                GetGroupID())) << type <<
            value << static_cast<int>(time(nullptr));
    _server.SendPacket(_monitorSession, buffer);
}
