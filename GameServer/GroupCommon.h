#pragma once
#include "ContentTypes.h"
#include "Group.h"
#include "../Sector.h"
#include "../GameMap.h"
#include "../Data/ServerInitData.h"



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

        void SetUseDB(const bool use)
        {
            _useDB = use;
        }

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
        bool _useDB = true;
        std::chrono::steady_clock::time_point _nextDBSend{};
        std::chrono::steady_clock::time_point _prevUpdate{};

        //Monitor
        long _groupSessionCount = 0;
        long _fps = 0;

        static SessionID _monitorSession;
        static String _monitorIp;
        static Port _monitorPort;

    };
}
