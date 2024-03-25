#pragma once
#include <shared_mutex>

#include "ContentTypes.h"
#include "IOCP.h"
#include "SettingParser.h"
#include "ServerInitData.h"

class DBConnection;

namespace psh
{
    class DBData;
    class Player;

    class Server :
        public IOCP
    {
        struct defaultSetting
        {
            
        };

        
    public:
        Server();
        void OnConnect(SessionID sessionId, const SockAddr_in& info) override;
        void OnDisconnect(SessionID sessionId) override;

        void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
        void OnMonitorRun() override;

        shared_ptr<DBData> getDbData(SessionID id);

        bool MoveDebug() const
        {
            return _moveDebug;
        }

        GroupID GetGroupID(ServerType type) const
        {
            return _groups[static_cast<int>(type)];
        }

    private:
        void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnLogin(SessionID sessionId, CRecvBuffer& buffer);

        [[nodiscard]] DBConnection* GetGameDbConnection();
        
    private:
        //1 : dbData, 2 : dbConnection
        USE_MANY_LOCKS(2)

        SessionMap<shared_ptr<DBData>> g_dbData;

        vector<GroupID> _groups;
        bool _moveDebug = false;

    SettingParser serverSettings;
        ServerInitData _initData;
        int DBTLSId;
    };
}
