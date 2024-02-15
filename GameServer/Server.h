#pragma once
#include "ContentTypes.h"
#include "IOCP.h"




namespace psh 
{
    class Player;
    class GameMap;
    
    class Server :
        public IOCP
    {
    public:
        Server();
        void OnConnect(SessionID sessionId, const SockAddr_in& info) override;
        void OnDisconnect(SessionID sessionId) override;
        void OnAttack(SessionID sessionId, CRecvBuffer& buffer);
        void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;

        void OnMonitorRun() override;
    private:
        void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnMove(SessionID sessionId,CRecvBuffer& buffer);
    private:
        AccountNo g_AccountNo = 0;
        SessionMap<shared_ptr<Player>> _players;
        unique_ptr<GameMap> _gameMap;
    };

}


