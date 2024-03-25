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
    _objectManager = make_unique<ObjectManager>(*this, *_playerMap);

    if (_useDB)
    {
        _dbThread = make_unique<DBThreadWrapper>(
                                                 data.gameDBIP.c_str()
                                                 , data.gameDBPort
                                                 , data.gameDBID.c_str()
                                                 , data.gameDBPwd.c_str()
                                                 , "mydb");
    }
}


psh::GroupCommon::~GroupCommon() = default;

void psh::GroupCommon::SendInRange(FVector location
                                   , std::span<const Sector> offsets
                                   , SendBuffer& buffer
                                   , const shared_ptr<Player>& exclude)
{
    auto broadcastSectors = _playerMap->GetSectorsFromOffset(_playerMap->GetSector(location), offsets);
    ranges::for_each(broadcastSectors, [ this,&buffer,&exclude](flat_unordered_set<shared_ptr<Player>>& sector)
    {
        for (auto& player : sector)
        {
            if (player == exclude)
            {
                continue;
            }

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

void psh::GroupCommon::OnLeave(SessionID id)
{
    auto it = _players.find(id);
    auto& [_,playerPtr] = *it;

    if (playerPtr->InMap())
    {
        _objectManager->RemoveFromMap(playerPtr, playerPtr->Location(), SEND_OFFSETS::BROADCAST, false, false
                                      , ObjectManager::removeResult::GroupChange);
        playerPtr->_data->SetLocation(playerPtr->Location());
        _dbThread->LeaveGroup(playerPtr->_data, &_server, id, GroupManager::BaseGroupID());
    }

    _players.erase(it);
}


void psh::GroupCommon::OnUpdate(int milli)
{
    if (milli > 200)
    {
        milli = 200;
    }

    UpdateContent(milli);
    _objectManager->CleanupDestroyWait();
    _fps++;

    if (std::chrono::steady_clock::now() < _nextDBSend)
    {
        return;
    }
    if (!_useDB)
    {
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


    _objectManager->RemoveFromMap(playerPtr, playerPtr->Location(), SEND_OFFSETS::BROADCAST, false, false
                                  , ObjectManager::removeResult::GroupChange);

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

void psh::GroupCommon::RecvMove(SessionID sessionId, CRecvBuffer& buffer)
{
    auto& [_,player] = *_players.find(sessionId);
    FVector location;
    GetGame_ReqMove(buffer, location);
    if (player == nullptr)
    {
        _iocp->DisconnectSession(sessionId);
    }

    player->MoveStart(location);
}

void psh::GroupCommon::RecvAttack(SessionID sessionId, CRecvBuffer& buffer)
{
    auto& [_,player] = *_players.find(sessionId);
    char type;
    GetGame_ReqAttack(buffer, type);

    if (player == nullptr)
    {
        _iocp->DisconnectSession(sessionId);
    }
    player->Attack(type);
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
    auto monitor = _dbThread->GetMonitor();
    
    printf("Players : %zd\n"
        "Group : %d, Work : %lld, Queue: %d, Handled : %lld\n"
           "DBQueued: %lld, DBEnqueue : %lld, DBDequeue : %lld, DBDelayAvg : %f\n"
        ,_players.size()
           , int(GetGroupID()), GetWorkTime(), GetQueued(), GetJobTps()
           ,monitor.queued,monitor.enqueue,monitor.dequeue,monitor.delaySum / float(monitor.dequeue));

    //_playerMap->PrintPlayerInfo();
};
