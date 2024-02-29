#include "EasyMonsterGroup.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "../GameMap.h"
#include "../Server.h"
#include "../Base/Player.h"
#include "Profiler.h"

namespace psh
{
    EasyMonsterGroup::EasyMonsterGroup(Server* server,short mapSize, short sectorSize): GroupCommon(server, ServerType::Easy,mapSize,sectorSize)
    ,_monsterMap(make_unique<GameMap>(mapSize,mapSize,server)) {}

    void EasyMonsterGroup::OnEnter(SessionID id)
    {
        auto playerPtr = _server->getPlayerPtr(id);
        _players.emplace(id,playerPtr);

        auto levelInfoPacket = SendBuffer::Alloc();
        MakeGame_ResLevelEnter(levelInfoPacket,playerPtr->AccountNumber(),playerPtr->ObjectId(), _groupType);
        SendPacket(playerPtr->SessionId(), levelInfoPacket);
    }

    void EasyMonsterGroup::OnLeave(const SessionID id)
    {

        auto it = _players.find(id);
        auto& [_,playerPtr] = *it;
        
        _playerMap->BroadcastDisconnect(playerPtr);
        _players.erase(it);
    }



    void EasyMonsterGroup::OnRecv(const SessionID id, CRecvBuffer& recvBuffer)
    {
        ePacketType type;
        recvBuffer >> type;
        switch (type)
        {
        case eGame_ReqChangeComplete:
            OnChangeComp(id,recvBuffer);
            break;
        case eGame_ReqMove:
            OnMove(id,recvBuffer);
            break;
        case eGame_ReqAttack:
            OnAttack(id,recvBuffer);
            break;
        case eGame_ReqLevelChange:
            OnReqLevelChange(id,recvBuffer);
            break;
        default:
            DebugBreak();
            break;
        }
    }
    
    void EasyMonsterGroup::OnChangeComp(const SessionID id, CRecvBuffer& recvBuffer)
    {
        AccountNo accountNo;
        GetGame_ReqChangeComplete(recvBuffer,accountNo);
        
        auto& [_,playerPtr] = *_players.find(id);
        
        _playerMap->SpawnPlayer(playerPtr);
    }

    void EasyMonsterGroup::OnReqLevelChange(const SessionID id, CRecvBuffer& recvBuffer) const
    {
        AccountNo accountNo;
        ServerType type;
        GetGame_ReqLevelChange(recvBuffer,accountNo,type);
        
        MoveSession(id,_server->GetGroupID(type));
    }

    void EasyMonsterGroup::UpdateContent(const float delta)
    {
        
        
        for(auto& [_,actor] : _players)
        {
            actor->Update(delta);
        }
    }

    void EasyMonsterGroup::SendMonitor()
    {
    }

    EasyMonsterGroup::~EasyMonsterGroup() = default;

    void EasyMonsterGroup::OnAttack(const SessionID sessionId, CRecvBuffer& buffer)
    {
        auto& [_,player] = *_players.find(sessionId);
        char type;
        GetGame_ReqAttack(buffer,type);
		
        if(player == nullptr)
        {
            _iocp->DisconnectSession(sessionId);
        }

        player->Attack(type);
    }
    
    void EasyMonsterGroup::OnMove(const SessionID sessionId, CRecvBuffer& buffer)
    {
        //printf(format("Request isMove {:d} \n",sessionId.id).c_str());
        AccountNo accountNo;
        FVector location;
        GetGame_ReqMove(buffer,location);
        
        auto result = _players[sessionId];
        if(result == nullptr)
        {
            _iocp->DisconnectSession(sessionId);
        }

        location = Clamp(location,0,_playerMap->Size());
        
        result->MoveStart(location);
    }
}
