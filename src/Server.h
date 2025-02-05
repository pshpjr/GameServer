#pragma once
#include <shared_mutex>

#include <vector>
#include "CLogger.h"
#include "ContentTypes.h"
#include "IOCP.h"
#include "ServerInitData.h"
#include "SettingParser.h"
#include "ThreadPool.h"
enum en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE : BYTE;
class DBConnection;

namespace psh
{
	class DBData;
	class Player;
	class ChannelManager;

	//네트워크 송수신 담당
	//데이터베이스 정보 담당
	//TODO: DB 딴 쪽으로 빼기
	class Server final : public IOCP
	{
		struct DefaultSetting
		{
		};

	public:
		Server();
		~Server() override;
		void OnConnect(SessionID sessionId, const SockAddr_in& info) override;
		void OnDisconnect(SessionID sessionId, int wsaErrCode) override;

		void OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer) override;
		void OnMonitorRun() override;

		std::shared_ptr<DBData> GetDbData(SessionID id);

		bool MoveDebug() const
		{
			return _moveDebug;
		}

		[[nodiscard]] GroupID GetGroupId(ServerType type) const;

	private:
		void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value);


		void SendLogin();


		void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
		void OnLogin(SessionID sessionId, CRecvBuffer& buffer);

		[[nodiscard]] DBConnection& GetGameDbConnection();

		//1 : dbData, 2 : dbConnection
		USE_MANY_LOCKS(2)

		CLogger _connectionLogger{L"Server", CLogger::LogLevel::Debug};
		SessionMap<std::shared_ptr<DBData>> _dbData;

		std::unique_ptr<ChannelManager> _channelManager;

		bool _moveDebug = false;

		SettingParser _serverSettings;
		ServerInitData _initData;
		DWORD _dbTlsId;
		std::atomic<uint64> _dbErrorCount;
		SessionID _monitorSession = InvalidSessionID();
		ThreadPool _threadPool{1};
	};
}
