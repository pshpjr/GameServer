#pragma once
#include <shared_mutex>

#include <vector>
#include "CLogger.h"
#include "ContentTypes.h"
#include "IOCP.h"
#include "ServerInitData.h"
#include "SettingParser.h"
class DBConnection;

namespace psh
{
    class DBData;
    class Player;

    struct MonitorData
    {
        GroupID id{0};
        int64 workTime{};
        int64 queued{};
        int64 jobTps{};
        int32 dbError{};
    };

    class Server final : public IOCP
    {
        struct DefaultSetting {};

    public:
        Server();
        void OnConnect(SessionID sessionId, const SockAddr_in& info) override;
        void OnDisconnect(SessionID sessionId, int wsaErrCode) override;

        void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
        void OnMonitorRun() override;

        std::shared_ptr<DBData> GetDbData(SessionID id);

        bool MoveDebug() const
        {
            return _moveDebug;
        }

        [[nodiscard]] GroupID GetGroupId(ServerType type) const
        {
            return _groups[static_cast<int>(type)];
        }

    private:
        void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnLogin(SessionID sessionId, CRecvBuffer& buffer);

        [[nodiscard]] DBConnection& GetGameDbConnection();

        //1 : dbData, 2 : dbConnection
        USE_MANY_LOCKS(2)

        CLogger _connectionLogger;
        SessionMap<std::shared_ptr<DBData>> _dbData;

        std::vector<GroupID> _groups{};
        std::vector<MonitorData> _monitors{};
        bool _moveDebug = false;

        SettingParser _serverSettings;
        ServerInitData _initData;
        DWORD _dbTlsId;
        std::atomic<uint64> _dbErrorCount;
    };
}
