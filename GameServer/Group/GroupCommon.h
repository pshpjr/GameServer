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
        GroupCommon(Server* server, ServerType type,short mapSize = 6400, short sectorSize = 800):
        _server(server) ,_groupType(type),_prevUpdate(std::chrono::steady_clock::now()),
        _playerMap(make_unique<GameMap>(mapSize,sectorSize,server)){}
        
        void SetUseDB(const bool use){_useDB = use;}
        
        void OnUpdate() final;
        virtual void OnEnter(SessionID id) override = 0 ;
        virtual void OnLeave(SessionID id) override = 0;
        virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override = 0;
        virtual ~GroupCommon() override;
        
        virtual void UpdateContent(float deltaMs) = 0;
        virtual void SendMonitor() = 0;

        
    protected:
        Server* _server;
        //Info
        const ServerType _groupType = ServerType::End;
        unique_ptr<GameMap> _playerMap;
        
    private:
        
        //DB
        bool _useDB = false;
        chrono::steady_clock::time_point _nextDBSend {};
        chrono::steady_clock::time_point _prevUpdate {};
        
        //Monitor
        long _groupSessionCount = 0;
        long _fps = 0;

        static SessionID _monitorSession;
        static String _monitorIp;
        static Port _monitorPort;

    };
    

}
