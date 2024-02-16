#include "VillageGroup.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "../GameMap.h"
#include "../Player.h"
#include "../Server.h"

namespace psh
{
    void psh::VillageGroup::OnEnter(SessionID id)
    {
        auto playerPtr = _server->getPlayerPtr(id);
        _players.emplace(id,playerPtr);

        auto levelInfoPacket = SendBuffer::Alloc();
        MakeGame_ResChangeLevel(levelInfoPacket,playerPtr->AccountNumber(), _serverType);
        SendPacket(playerPtr->SessionId(), levelInfoPacket);
    }

    void psh::VillageGroup::OnLeave(SessionID id)
    {
        auto it = _players.find(id);
        auto& [_,playerPtr] = *it;
        
        _gameMap->DestroyPlayer(playerPtr);
        _players.erase(it);
    }



    void psh::VillageGroup::OnRecv(SessionID id, CRecvBuffer& recvBuffer)
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
        default:
            DebugBreak();
            break;
        }
    }
    
    void VillageGroup::OnChangeComp(SessionID id, CRecvBuffer& recvBuffer)
    {
        AccountNo accountNo;
        GetGame_ReqChangeComplete(recvBuffer,accountNo);
        
        auto& [_,playerPtr] = *_players.find(id);
        
        _gameMap->SpawnPlayer(playerPtr, _serverType);
    }
    
    void psh::VillageGroup::UpdateContent(int delta)
    {
        for(auto& [_,player] : _players)
        {
            if(player->isMove())
            {
                auto normalDirection = (player->Destination() - player->Location()).Normalize();
                float DistanceToDestination = (player->Destination() - player->Location()).Size();

                if(DistanceToDestination < 10)
                {
                    _gameMap->MovePlayer(player,player->Destination());
                    player->MoveStop();
                }
                else
                {
                    _gameMap->MovePlayer(player, player->Location() + (normalDirection*10));
                }
            }
        }
        
    }

    void psh::VillageGroup::SendMonitor()
    {
    }

    psh::VillageGroup::~VillageGroup()
    {
    }
    
    void VillageGroup::OnAttack(SessionID sessionId, CRecvBuffer& buffer)
    {
        auto& [_,player] = *_players.find(sessionId);
        AccountNo account;
        GetGame_ReqAttack(buffer,account);
		
        if(player == nullptr)
        {
            _iocp->DisconnectSession(sessionId);
        }

        _gameMap->BroadcastAttack(player);
    }
    
    void psh::VillageGroup::OnMove(SessionID sessionId, CRecvBuffer& buffer)
    {
        printf(format("Request isMove {:d} \n",sessionId.id).c_str());
        AccountNo accountNo;
        FVector location;
        GetGame_ReqMove(buffer,accountNo,location);
        
        auto result = _players[sessionId];
        if(result == nullptr)
        {
            _iocp->DisconnectSession(sessionId);
        }

        _gameMap->BroadcastMoveStart(result,location);
        
        result->Destination(location);
        result->MoveStart();
        // _gameMap->
    }
}
