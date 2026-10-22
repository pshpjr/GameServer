#pragma once
#include <shared_mutex>

#include <vector>
#include "Logger.h"
#include "ContentTypes.h"
#include "IOCP.h"
#include "ServerInitData.h"
#include "SettingParser.h"
#include "ThreadPool.h"
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
		bool Init();
		~Server() override;
		void OnConnect(SessionID sessionId, const SockAddr& info) override;
		void OnDisconnect(SessionID sessionId, int wsaErrCode) override;

		void OnRecvPacket(SessionID sessionId, RecvBuffer& buffer) override;
		void OnMonitorRun() override;

		std::shared_ptr<DBData> GetDbData(SessionID id);

		bool MoveDebug() const
		{
			return _moveDebug;
		}

		[[nodiscard]] GroupID GetGroupId(ServerType type) const;

	private:
		void OnLoginLogin(SessionID sessionId, RecvBuffer& buffer);
		void OnLogin(SessionID sessionId, RecvBuffer& buffer);

		[[nodiscard]] DBConnection& GetGameDbConnection();

		//1 : dbData, 2 : dbConnection
		USE_MANY_LOCKS(2)

		Logger _connectionLogger{L"Server", Logger::LogLevel::Debug};
		SessionMap<std::shared_ptr<DBData>> _dbData;

		std::unique_ptr<ChannelManager> _channelManager;

		bool _moveDebug = false;

		SettingParser _serverSettings;
		ServerInitData _initData;
		util::TlsKey _dbTlsId;
		std::atomic<uint64> _dbErrorCount;
		ThreadPool _threadPool{1};
	};
}
