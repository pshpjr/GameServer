#include "Server.h"

#include <DBException.h>

#include <dpp/exception.h>

#include "DBConnection.h"
#include "DBData.h"
#include "Field.h"
#include "GameMap.h"
#include "LockGuard.h"
#include "MonitorProtocol.h"
#include "optick.h"
#include "PacketGenerated.h"
#include "Utility.h"
#include "ChannelManager.h"

namespace psh
{
	Server::Server()
		: _dbTlsId(TlsAlloc())
	{
		_serverSettings.Init(L"GameSettings.txt");
		_serverSettings.GetValue(L"db.GameDBIP", _initData.gameDBIP);
		_serverSettings.GetValue(L"db.GameDBPort", _initData.gameDBPort);
		_serverSettings.GetValue(L"db.GameDBID", _initData.gameDBID);
		_serverSettings.GetValue(L"db.GameDBPwd", _initData.gameDBPwd);
		_serverSettings.GetValue(L"db.MonitorIP", _initData.MonitorServerIP);
		_serverSettings.GetValue(L"db.MonitorPort", _initData.MonitorServerPort);
		_serverSettings.GetValue(L"db.useMonitorServer", _initData.UseMonitorServer);

		_serverSettings.GetValue(L"game.useConsoleMonitor", _initData.consoleMonitor);

		_channelManager = std::make_unique<ChannelManager>(this, _groupManager.get(), _initData);
	}

	Server::~Server() = default;

	void Server::OnConnect(SessionID sessionId, const SockAddr_in& info)
	{
		//printf(format("Connect {:d} \n",sessionId.id).c_str());
	}

	void Server::OnDisconnect(SessionID sessionId, int wsaErrCode)
	{
		//printf(format("Disconnect {:d} \n",sessionId.id).c_str());

		AccountNo accountNo;
		{
			WRITE_LOCK;
			const auto it = _dbData.find(sessionId);
			if (it == _dbData.end())
			{
				return;
			}
			//shared_ptr을 복사하지 않고 처리하고 싶었음. 이 블럭 내부에서만 사용할거니까
			const auto& dbData = it->second;
			accountNo = dbData->AccountNum();
			_dbData.erase(it);
		}
		_threadPool.enqueue([accountNo, this]
		{
			try
			{
				//나중에 계정이 엄청나게 많아진다면 파티션 걸어서 성능 올릴 수 있을까?
				auto& conn = GetGameDbConnection();
				conn.QueryFormat("Update account set LoginState = 0 where (AccountNo = {})", accountNo);
				conn.reset();
			}
			catch (std::exception& e)
			{
				_connectionLogger.Write(L"DBError", CLogger::LogLevel::Debug, L"DBErr OnDisconnect  : %s"
				                        , e.what());
			}
		});
	}

	void Server::OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer)
	{
		ePacketType type;
		buffer >> type;

		switch (type)
		{
		case None:
			DebugBreak();
			break;
		case eLogin_ReqLogin:
			OnLoginLogin(sessionId, buffer);
			break;
		case eLogin_ReqRegister:
			break;
		case eGame_ReqLogin:
			OnLogin(sessionId, buffer);
			break;
		default:
			__debugbreak();
			break;
		}
	}

	void Server::OnMonitorRun()
	{
		if (GetAsyncKeyState('P') & 0x8000)
		{
			std::cout << "START CAPUTRE\n";
			OPTICK_START_CAPTURE();
		}

		if (GetAsyncKeyState('S') & 0x8000)
		{
			std::cout << "END CAPUTRE\n";
			OPTICK_STOP_CAPTURE();
			OPTICK_SAVE_CAPTURE("dup.opt");
		}


		if (_initData.consoleMonitor)
		{
			PrintMonitorString();
		}

		if (_initData.UseMonitorServer)
		{
			if (_monitorSession == InvalidSessionID())
			{
				auto client = GetClientSession(_initData.MonitorServerIP, _initData.MonitorServerPort);
				if (client.HasError())
				{
					return;
				}
				_monitorSession = client.Value();
				SetSessionStaticKey(_monitorSession, 0);
				SendLogin();
			}
			else
			{
				SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_ACCEPT_TPS, GetAcceptTps());
				SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_SEND_TPS, GetSendTps());
				SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_RECV_TPS, GetRecvTps());
				SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_DISCONNECT_TPS, GetDisconnectPerSec());
				SendMonitorData(dfMONITOR_DATA_TYPE_GAME_SERVER_SESSIONS, GetSessions());
			}
		}


		if (GetAsyncKeyState(VK_HOME))
		{
			Stop();
		}
	}

	std::shared_ptr<DBData> Server::GetDbData(SessionID id)
	{
		//없으면 알아서 터짐.

		READ_LOCK;
		auto& ret = _dbData.find(id)->second;

		return ret;
	}

	GroupID Server::GetGroupId(ServerType type) const
	{
		return _channelManager->GetGroupIDByType(type);
	}

	void Server::SendMonitorData(const en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, const int value)
	{
		auto buffer = SendBuffer::Alloc();
		buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << static_cast<WORD>(1) <<
			static_cast<char>(0) << type << value << static_cast<int>(time(nullptr));
		SendPacket(_monitorSession, buffer);
	}

	void Server::SendLogin()
	{
		auto buffer = SendBuffer::Alloc();
		buffer << en_PACKET_SS_MONITOR_LOGIN << static_cast<WORD>(1) << static_cast<char>(0);
		SendPacket(_monitorSession, buffer);
	}

	//로그인 서버 겸용으로 쓰다가 나중에 제거.
	void Server::OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer)
	{
		//printf(format("Login to LoginServer {:d} \n", sessionId.id).c_str());
		using namespace psh;

		ID playerID;
		Password playerPass;
		GetLogin_ReqLogin(buffer, playerID, playerPass);
		const String id = playerID.ToString();
		const std::string cid = util::WToS(id);

		_threadPool.enqueue([cid, playerID, playerPass, sessionId, this]
		{
			try
			{
				auto& conn = GetGameDbConnection();
				//ID를 인덱스 설정 안 했었지만, UQ라 자동 생성된 인덱스 타고 있었음.
				//account 테이블의 클러스터 인덱스를 ID로 건다면?
				//지금은 로그인에서 1회 ID 조회, 로그아웃에서 Account 1회 조회 하고 있음. 접근 횟수가 작으니까 키 크기 작은걸로
				conn.QueryFormat("select AccountNo, ID, PASS,LoginState from account where ID = '{}'", cid);

				auto loginResult = SendBuffer::Alloc();

				if (!conn.next())
				{
					MakeLogin_ResLogin(loginResult, 0, playerID, eLoginResult::InvalidId, SessionKey());
				}
				else
				{
					const AccountNo accountNo = conn.getInt(0);
					//ID trueID(conn->getString(1));
					const Password truePass(conn.getString(2));
					const bool loginState(conn.getChar(3));
					conn.reset();

					if (truePass != playerPass)
					{
						MakeLogin_ResLogin(loginResult, 0, playerID, eLoginResult::WrongPassword, SessionKey());
					}
					else if (loginState == true)
					{
						MakeLogin_ResLogin(loginResult, 0, playerID, eLoginResult::DuplicateLogin, SessionKey());
					}
					else
					{
						MakeLogin_ResLogin(loginResult, accountNo, playerID, eLoginResult::LoginSuccess, SessionKey());
					}
				}
				SetTimeout(sessionId, 30000);
				SendPacket(sessionId, loginResult);
			}
			catch (const DBErr& e)
			{
				DisconnectSession(sessionId);
				size_t requiredSize = 0;
				mbstowcs_s(&requiredSize, nullptr, 0, e.what(), 0);

				std::wstring result(requiredSize, L'\0');

				// 실제 문자열 변환 수행
				size_t convertedChars = 0;
				mbstowcs_s(&convertedChars, &result[0], requiredSize, e.what(), _TRUNCATE);
				_connectionLogger.Write(L"DBError", CLogger::LogLevel::Debug, L"DBErr disconnect LoginLogin : %s"
				                        , result.c_str());
			}
		});
	}

	void Server::OnLogin(SessionID sessionId, CRecvBuffer& buffer)
	{
		//printf(format("Login to GameServer {:d} \n", sessionId.id).c_str());
		using namespace psh;
		AccountNo AccountNo;
		{
			SessionKey key;
			//게임 로그인은 무조건 성공
			GetGame_ReqLogin(buffer, AccountNo, key);


			auto loginResult = SendBuffer::Alloc();

			MakeGame_ResLogin(loginResult, AccountNo, true);
			SendPacket(sessionId, loginResult);
		}


		_threadPool.enqueue([AccountNo, sessionId, this]
		{
			auto& conn = GetGameDbConnection();

			try
			{
				//(PlayerNo, accountNo)가 키라서 인덱스 안 탈줄 알았는데 accountNo가 fk라 인덱스 생겨 있었음.
				conn.QueryFormat(
					"select Nick,HP,Coins,CharType,ServerType,LocationX,LocationY from mydb.player where AccountNo = {}"
					, AccountNo);

				if (!conn.next())
				{
					_dbErrorCount.fetch_add(1, std::memory_order_relaxed);
					_connectionLogger.Write(L"DB", CLogger::LogLevel::Debug, L"DBFail : %d ", AccountNo);
					DisconnectSession(sessionId);
					return;
				}

				Nickname nick(conn.getString(0));
				int hp = conn.getInt(1);
				int coins = conn.getInt(2);
				char charType = conn.getChar(3);
				char serverType = conn.getChar(4);
				FVector location = {conn.getFloat(5), conn.getFloat(6)};

				conn.reset();

				bool result = false;
				{
					WRITE_LOCK;
					result = _dbData.emplace(sessionId, std::make_shared<DBData>(sessionId, AccountNo, location
						                         , serverType, charType, coins, hp
						                         , nick)).second;
				}
				if (result == false)
				{
					_connectionLogger.Write(L"DB", CLogger::LogLevel::Debug, L"Cannot create DBData. %d %d %s"
					                        , AccountNo
					                        , serverType, nick.ToString());
					DisconnectSession(sessionId);
					//플레이어 생성에 실패한 관련 에러 처리.
				}
				else
				{
					conn.QueryFormat("Update account set LoginState = 1 where (AccountNo = {})", AccountNo);
					conn.reset();
				}


				if (hp <= 0)
				{
					//마을로 보낸다.
					_groupManager->MoveSession(
						sessionId, GetGroupId(ServerType::Village));
				}
				else
				{
					_groupManager->
						MoveSession(sessionId, GetGroupId(static_cast<ServerType>(serverType)));
				}
			}
			catch (const std::exception& e)
			{
				DisconnectSession(sessionId);
				std::cout << "DBFail " << AccountNo << std::endl;
				_connectionLogger.Write(L"DBError", CLogger::LogLevel::System, L"DBErr disconnect GameLogin : %s"
				                        , e.what());
				conn.reset();
			}
		});
	}

	DBConnection& Server::GetGameDbConnection()
	{
		void* tlsValue = TlsGetValue(_dbTlsId);

		if (tlsValue == nullptr)
		{
			{
				WRITE_LOCK_IDX(1);
				tlsValue = static_cast<void*>(new DBConnection(_initData.gameDBIP.c_str(), _initData.gameDBPort
				                                               , _initData.gameDBID.c_str()
				                                               , _initData.gameDBPwd.c_str(), "mydb"));
			}
			TlsSetValue(_dbTlsId, tlsValue);
		}
		return *static_cast<DBConnection*>(tlsValue);
	}
}
