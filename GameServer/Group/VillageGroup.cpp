#include "VillageGroup.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "../GameMap.h"
#include "../base/Player.h"
#include "../Server.h"
#include "Profiler.h"

namespace psh
{
    void psh::VillageGroup::OnEnter(SessionID id)
    {
        auto playerPtr = _server->getPlayerPtr(id);
        _players.emplace(id,playerPtr);
        _iocp->SetTimeout(id, 30000);
        
        playerPtr->SetGroup(this);
        auto levelInfoPacket = SendBuffer::Alloc();
        MakeGame_ResLevelEnter(levelInfoPacket,playerPtr->AccountNumber(),playerPtr->ObjectId(), _groupType);
        SendPacket(playerPtr->SessionId(), levelInfoPacket);
    }

    void psh::VillageGroup::OnLeave(const SessionID id)
    {
        auto it = _players.find(id);
        auto& [_,playerPtr] = *it;
        
        _playerMap->BroadcastDisconnect(playerPtr);
        _players.erase(it);
    }



    void psh::VillageGroup::OnRecv(const SessionID id, CRecvBuffer& recvBuffer)
    {
        psh::ePacketType type;
        recvBuffer >> type;
        switch (type)
        {
        case psh::ePacketType::eGame_ReqChangeComplete:
            OnChangeComp(id,recvBuffer);
            break;
        case psh::eGame_ReqMove:
            OnMove(id,recvBuffer);
            break;
        case psh::ePacketType::eGame_ReqAttack:
            OnAttack(id,recvBuffer);
            break;
        case psh::ePacketType::eGame_ReqLevelChange:
            OnReqLevelChange(id,recvBuffer);
            break;
        default:
            DebugBreak();
            break;
        }
    }
    
    void VillageGroup::OnChangeComp(const SessionID id, CRecvBuffer& recvBuffer)
    {
        AccountNo accountNo;
        GetGame_ReqChangeComplete(recvBuffer,accountNo);
        
        auto& [_,playerPtr] = *_players.find(id);
        
        _playerMap->SpawnPlayer(playerPtr);
    }

    void VillageGroup::OnReqLevelChange(const SessionID id, CRecvBuffer& recvBuffer) const
    {
        AccountNo accountNo;
        ServerType type;
        GetGame_ReqLevelChange(recvBuffer,accountNo,type);
        
        MoveSession(id,_server->GetGroupID(type));
    }

    void psh::VillageGroup::UpdateContent(const float delta)
    {
        
        for(auto& [_,actor] : _players)
        {
            actor->Update(delta);
        }
        
    }

    void psh::VillageGroup::SendMonitor()
    {
    }

    psh::VillageGroup::~VillageGroup()
    {
    }
    
    void VillageGroup::OnAttack(const SessionID sessionId, CRecvBuffer& buffer)
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
    
    void psh::VillageGroup::OnMove(const SessionID sessionId, CRecvBuffer& buffer)
    {
        //printf(format("Request isMove {:d} \n",sessionId.id).c_str());
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
