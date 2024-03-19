#include "GroupCommon.h"

#include "../Base/ObjectManager.h"
#include "../Base/Player.h"
#include "../Server.h"
#include "../Data/TableData.h"
#include "../Base/AttackManager.h"

psh::GroupCommon::GroupCommon(Server& server, ServerType type
    , short mapSize, short sectorSize):
        _server(server) ,_groupType(type), 
        _prevUpdate(std::chrono::steady_clock::now())
,_nextDBSend(chrono::steady_clock::now())
{
    _playerMap = make_shared<GameMap<shared_ptr<Player>>>(mapSize, sectorSize);
    _objectManager = make_unique<ObjectManager>(*this,*_playerMap);   
}

psh::GroupCommon::~GroupCommon() = default;

void psh::GroupCommon::SendInRange(FVector location
                                   , std::span<const Sector> offsets
                                   , SendBuffer& buffer
                                   , const shared_ptr<psh::Player>& exclude)
{
    auto broadcastSectors = _playerMap->GetSectorsFromOffset(_playerMap->GetSector(location), offsets);
    ranges::for_each(broadcastSectors, [ this,&buffer,&exclude](flat_unordered_set<shared_ptr< psh::Player>> sector)
    {
        for (auto player : sector)
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

    auto playerPtr = make_unique<Player>(_objectManager->NextObjectId(),*_objectManager,*this, dataPtr->Location(), *dataPtr);

    //TODO: DB 사용시 제거.
    auto levelInfoPacket = SendBuffer::Alloc();
    MakeGame_ResLevelEnter(levelInfoPacket,playerPtr->AccountNumber(),playerPtr->ObjectId(), _groupType);
    SendPacket(playerPtr->SessionId(), levelInfoPacket);
    

    _players.insert({id, std::move(playerPtr)});
    _iocp->SetTimeout(id, 30000);

}

void psh::GroupCommon::OnLeave(SessionID id)
{
    auto it = _players.find(id);
    auto& [_,playerPtr] = *it;

    if(!playerPtr->isDead())
    {
        _objectManager->DestroyActor(playerPtr,playerPtr->Location(),SEND_OFFSETS::BROADCAST,false,false,2);
    }
    
    _players.erase(it);
}


void psh::GroupCommon::OnUpdate(int milli)
{
    if(milli > 200)
    {
        milli = 200;
    }
    
    UpdateContent(milli);
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

    MoveSession(id, _server.GetGroupID(type));
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

    _objectManager->SpawnActor(playerPtr,_attackManager.get());
}

void psh::GroupCommon::RecvMove(SessionID sessionId, CRecvBuffer& buffer)
{
    auto& [_,player] = *_players.find(sessionId);
    FVector location;
    GetGame_ReqMove(buffer,location);
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
        actor->Update(deltaMs);
    }
    
    _objectManager->Update(deltaMs);
}

void psh::GroupCommon::SendMonitor()
{
    //printf("Group %d , Players : %d , mapPlayer : %d\n",GetGroupID(),_players.size(), _playerMap->Players());
};