#pragma once
#include "Group.h"
#include "../GameMap.h"

namespace psh
{
    class Server;
    class GameMap;
    class Player;

    class GroupCommon : public Group
    {
    protected:


        
    public:
        GroupCommon(Server* server, ServerType type,
            const int mapSize, const int sectorSize):_server(server),_gameMap(make_unique<GameMap>(mapSize,sectorSize,_server,type)) ,_serverType(type)  {  }
        void SetUseDB(bool use){_useDB = use;}
        
        void OnUpdate() final;
        virtual void OnEnter(SessionID id) override = 0 ;
        virtual void OnLeave(SessionID id) override = 0;
        virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override = 0;
        virtual ~GroupCommon() override;


        
        virtual void UpdateContent(int delta) = 0;
        virtual void SendMonitor() = 0;


    protected:
        Server* _server;
        //content
        SessionMap<shared_ptr<Player>> _players;
        unique_ptr<GameMap> _gameMap;
        

        //Info
        const ServerType _serverType = ServerType::End;
        const GroupID _serverGroup = GroupID(1);

    private:

        //DB
        bool _useDB = false;
        chrono::steady_clock::time_point _nextDBSend {};
        
        //Monitor
        long _groupSessionCount = 0;
        long _fps = 0;

        static SessionID _monitorSession;
        static String _monitorIp;
        static Port _monitorPort;
    };
    

}
