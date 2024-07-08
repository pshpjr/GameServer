#include "GroupCommon.h"

#include "Profiler.h"
#include "ObjectManager.h"
#include "Player.h"
#include "Server.h"
#include "TableData.h"
#include "DBThreadWrapper.h"

psh::GroupCommon::GroupCommon(Server& server
                              , const ServerInitData& data
                              , ServerType type
                              , short mapSize
                              , short sectorSize):
                                                 _server(server)
                                                 , _initData(data)
                                                 , _groupType(type)
                                                 , _nextDBSend(chrono::steady_clock::now())
                                                 , _prevUpdate(std::chrono::steady_clock::now())

{
    _playerMap = make_shared<GameMap<shared_ptr<Player>>>(mapSize, sectorSize);
    _objectManager = make_unique<ObjectManager>(*this, *_playerMap,nullptr);

    _useMonitor = _initData.UseMonitorServer;

        _dbThread = make_unique<DBThreadWrapper>(
                                                 data.gameDBIP.c_str()
                                                 , data.gameDBPort
                                                 , data.gameDBID.c_str()
                                                 , data.gameDBPwd.c_str()
                                                 , "mydb");
    
    
}


psh::GroupCommon::~GroupCommon() = default;

void psh::GroupCommon::SendInRange(FVector location
                                   , std::span<const Sector> offsets
                                   , SendBuffer& buffer
                                   , const shared_ptr<Player>& exclude)
{
    auto broadcastSectors = _playerMap->GetSectorsFromOffset(_playerMap->GetSector(location), offsets);
    ranges::for_each(broadcastSectors, [ this,&buffer,&exclude](auto& sector)
    {
        for (auto& player : sector)
        {
            if (player == exclude)
            {
                continue;
            }
            //printf("SendTo Player : objid : %d\n", player->ObjectId());
            SendPacket(player->SessionId(), buffer);
        }
    });
}

void psh::GroupCommon::OnEnter(SessionID id)
{
    auto dataPtr = _server.getDbData(id);

    if (static_cast<ServerType>(dataPtr->ServerType()) != _groupType)
    {
        dataPtr->SetServerType(static_cast<char>(_groupType));
        dataPtr->SetLocation(_playerMap->GetRandomLocation());
    }

    auto playerPtr = make_unique<Player>(_objectManager->NextObjectId(), *_objectManager, *this, dataPtr->Location()
                                         , dataPtr,_dbThread.get());


    
    auto levelInfoPacket = SendBuffer::Alloc();
    MakeGame_ResLevelEnter(levelInfoPacket, playerPtr->AccountNumber(), playerPtr->ObjectId(), _groupType);
    SendPacket(playerPtr->SessionId(), levelInfoPacket);
    
    _players.insert({id, std::move(playerPtr)});

    _iocp->SetTimeout(id, 30000);
}

void psh::GroupCommon::OnLeave(SessionID id, int wsaErrCode)
{
    auto it = _players.find(id);
    auto& [_,playerPtr] = *it;

    if (playerPtr->InMap())
    {
        //Destroy 쓰면 나가고 맞는 상황이 생김. leaveGroup 이후 값의 변동이 생길 수 있음. 
        _objectManager->RemoveFromMap(playerPtr, playerPtr->Location(), SEND_OFFSETS::BROADCAST, false, false
                                      , ObjectManager::removeResult::GroupChange);
        playerPtr->_data->SetLocation(playerPtr->Location());
        _dbThread->LeaveGroup(playerPtr->_data, &_server, id, GroupManager::BaseGroupID());
    }

    _players.erase(it);
}


void psh::GroupCommon::OnUpdate(int milli)
{

    UpdateContent(milli);
    _objectManager->CleanupDestroyWait();
    _fps++;

    if (std::chrono::steady_clock::now() < _nextDBSend)
    {
        return;
    }

    SendMonitor();

    _nextDBSend += 1s;
    _fps = 0;
}

void psh::GroupCommon::OnRecv(SessionID id, CRecvBuffer& recvBuffer)
{
    ePacketType type;
    recvBuffer >> type;
    auto& [_,playerPtr] = *_players.find(id);
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

void psh::GroupCommon::RecvReqLevelChange(SessionID id, CRecvBuffer& recvBuffer) const
{
    AccountNo accountNo;
    ServerType type;
    GetGame_ReqLevelEnter(recvBuffer, accountNo, type);

    auto it = _players.find(id);
    auto& [_,playerPtr] = *it;


    if (playerPtr->InMap())
    {
    _objectManager->RemoveFromMap(playerPtr, playerPtr->Location(), SEND_OFFSETS::BROADCAST, false, false
                                  , ObjectManager::removeResult::GroupChange);
    }


    playerPtr->_data->SetLocation(playerPtr->Location());

    _dbThread->LeaveGroup(playerPtr->_data,&_server,id,_server.GetGroupID(type));
}

void psh::GroupCommon::RecvChangeComp(SessionID id, CRecvBuffer& recvBuffer)
{
    AccountNo accountNo;
    GetGame_ReqChangeComplete(recvBuffer, accountNo);

    auto& [_,playerPtr] = *_players.find(id);

    if (playerPtr->isDead())
    {
        playerPtr->Revive();
    }
    _dbThread->EnterGroup(playerPtr->_data);
    _objectManager->SpawnActor(playerPtr, _attackManager.get());
}

void psh::GroupCommon::RecvChat(SessionID id, CRecvBuffer& recvByffer) 
{
    auto& [_, player] = *_players.find(id);
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

    SendInRange(player->Location(), SEND_OFFSETS::BROADCAST, chatBuffer);
}

void psh::GroupCommon::RecvMove(SessionID sessionId, CRecvBuffer& buffer)
{
    auto& [_,player] = *_players.find(sessionId);
    FVector location;
    GetGame_ReqMove(buffer, location);

    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvMove. Not Found Player. SessionID : %d", sessionId);
        _iocp->DisconnectSession(sessionId);
        return;
    }

    //자기가 있는 위치로 이동해서 nan되는 문제 발생시 
    //단순히 리턴 하면 더미에서 move 패킷 검증올 못함. 
    //그거 방지용
    if (location == player->Location())
    {
        auto moveBuffer = SendBuffer::Alloc();
        MakeGame_ResMove(moveBuffer, player->ObjectId(), player->ObjectGroup(), player->Location());
        SendPacket(player->SessionId(), moveBuffer);
        printf("InvalidLocation. objID : %d, AccountNo : %lld\n", player->ObjectGroup(), player->AccountNumber());
        return;
    }
    player->MoveStart(location);

}

void psh::GroupCommon::RecvAttack(SessionID sessionId, CRecvBuffer& buffer)
{
    auto& [_,player] = *_players.find(sessionId);
    char type;
    psh::FVector dir;
    GetGame_ReqAttack(buffer, type,dir);

    if (player == nullptr)
    {
        _logger->Write(L"Disconnect", CLogger::LogLevel::Invalid, L"recvAttack. Not Found Player. SessionID : %d", sessionId);
        _iocp->DisconnectSession(sessionId);
    }
    if (player->isDead())
    {
        //printf("ResAttack playerDead, Account : %d\n", player->AccountNumber() );
        return;
    }

    player->Attack(type,dir);
}

void psh::GroupCommon::OnCreate()
{
    String extraName = L"Group";
    extraName += static_cast<int>(GetGroupID());
    _logger = make_unique<CLogger>(extraName.c_str());
}


void psh::GroupCommon::UpdateContent(int deltaMs)
{
    for (auto& [_,actor] : _players)
    {
        if (actor->NeedUpdate())
        {
            actor->Update(deltaMs);
        }
    }

    _objectManager->Update(deltaMs);
}

void psh::GroupCommon::SendMonitor()
{


    
    //printf("Players : %zd\n"
    //    "Group : %d, Work : %lld, Queue: %d, Handled : %lld\n"
    //     
    //    ,_players.size()
    //       , int(GetGroupID()), GetWorkTime(), GetQueued(), GetJobTps()
    //       );

    if (!_useMonitor)
    {
        return;
    }

    auto monitor = _dbThread->GetMonitor();
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


    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, int(1));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_WORK_TIME, static_cast<int>(GetWorkTime()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_JOB_QUEUE_SIZE, GetQueued());
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_JOB_TPS, static_cast<int>(GetJobTps()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SESSIONS, static_cast<int>(Sessions()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_PLAYERS, static_cast<int>(_players.size()));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_TPS, static_cast<int>(monitor.dequeue));
    SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_QUEUE_SIZE, static_cast<int>(monitor.queued));

    if (monitor.dequeue != 0) 
    {
        SendMonitorData(dfMONITOR_DATA_TYPE_GAME_DB_QUERY_AVG, static_cast<int>(monitor.delaySum / float(monitor.dequeue)));
    }
    
    
}
void psh::GroupCommon::SendLogin()
{
    auto buffer = SendBuffer::Alloc();

    buffer << en_PACKET_SS_MONITOR_LOGIN << WORD(1) << static_cast<char>(GetGroupID());
    _server.SendPacket(_monitorSession, buffer);
}

void psh::GroupCommon::SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value)
{
    auto buffer = SendBuffer::Alloc();

    buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << WORD(1) << static_cast<char>(GetGroupID()) << type << value << static_cast<int>( time(nullptr));
    _server.SendPacket(_monitorSession, buffer);
}
;
