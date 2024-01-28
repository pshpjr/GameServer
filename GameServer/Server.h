#pragma once
#include "IOCP.h"
#include "Player.h"
#include "GameMap.h"

namespace psh 
{
    class Server :
        public IOCP
    {
    public:
        Server();
        void OnConnect(SessionID sessionId, const SockAddr_in& info) override;
        void OnDisconnect(SessionID sessionId) override;
        void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
        void OnMonitorRun() override;
    private:
        void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnLogin(SessionID sessionId, CRecvBuffer& buffer);

    private:
        SessionMap<unique_ptr<Player>> _players;
        GameMap _map;
    };

}


