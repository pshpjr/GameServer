#pragma once
#include <shared_mutex>

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
        
        void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
        void OnMonitorRun() override;

        shared_ptr<Player> getPlayerPtr(SessionID id);
    private:
        void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnLogin(SessionID sessionId, CRecvBuffer& buffer);

        
    private:

        USE_LOCK
        
        AccountNo g_AccountNo = 0;
        SessionMap<shared_ptr<Player>> g_players;

        vector<GroupID> _groups;
    };

}


