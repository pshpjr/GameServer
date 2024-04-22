#pragma once
#include "ContentTypes.h"
#include "Group.h"
#include "Sector.h"
#include "GameMap.h"
#include "ServerInitData.h"
#include "MonitorProtocol.h"


struct ServerInitData;
class DBConnection;

struct ServerInitData;
class DBConnection;

namespace psh
{
    class DBThreadWrapper;
    class AttackManager;
    class Player;
    class Server;

    class GroupCommon : public Group
    {
    public:
        GroupCommon(Server& server,const ServerInitData& data, ServerType type, short mapSize = 6400, short sectorSize = 400);

        ~GroupCommon() override;
        void SendInRange(FVector location
                         , std::span<const Sector> offsets
                         , SendBuffer& buffer
                         , const shared_ptr<psh::Player>& exclude = nullptr);

        void OnEnter(SessionID id) final;
        void OnLeave(SessionID id) final;
        void OnUpdate(int milli) final;
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        void RecvReqLevelChange(SessionID id, CRecvBuffer& recvBuffer) const;
        void RecvChangeComp(SessionID id, CRecvBuffer& recvBuffer);
        void RecvMove(SessionID sessionId, CRecvBuffer& buffer);
        virtual void RecvAttack(SessionID sessionId, CRecvBuffer& buffer);
        
    protected:
        virtual void UpdateContent(int deltaMs);
        virtual void SendMonitor();
        unique_ptr<psh::DBThreadWrapper> _dbThread; 
        
        
        //Info
        Server& _server;
        const ServerInitData& _initData;
        const ServerType _groupType = ServerType::End;
        unique_ptr<psh::ObjectManager> _objectManager = nullptr;
        unique_ptr<psh::AttackManager> _attackManager = nullptr;

        SessionMap<shared_ptr<Player>> _players;
        shared_ptr<GameMap<shared_ptr<Player>>> _playerMap;
        
    private:
        //DB
        bool _useMonitor = false;
        chrono::steady_clock::time_point _nextDBSend{};
        chrono::steady_clock::time_point _prevUpdate{};

        //Monitor
        void SendLogin();
        void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value);



        long _groupSessionCount = 0;
        long _fps = 0;

        SessionID _monitorSession = InvalidSessionID();
    };
}
