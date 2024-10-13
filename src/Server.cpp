#include "Server.h"

#include "DBConnection.h"
#include "DBData.h"
#include "Field.h"
#include "GameMap.h"
#include "LockGuard.h"
#include "MonitorProtocol.h"
#include "optick.h"
#include "PacketGenerated.h"
#include "Utility.h"

namespace psh
{
    Server::Server()
        : _groups(static_cast<std::vector<GroupID>::size_type>(ServerType::End), GroupID::InvalidGroupID())
        , _monitors{static_cast<std::vector<GroupID>::size_type>(ServerType::End)}
        , _dbTlsId(TlsAlloc())
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

        _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Village)] = CreateGroup<Field>(*this, _initData
            , ServerType::Village, _monitors[static_cast<std::vector<GroupID>::size_type>(ServerType::Village)]);

        _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Easy)] = CreateGroup<Field>(*this, _initData
            , ServerType::Easy, _monitors[static_cast<std::vector<GroupID>::size_type>(ServerType::Easy)]);
        _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Hard)] = CreateGroup<Field>(*this, _initData
            , ServerType::Hard, _monitors[static_cast<std::vector<GroupID>::size_type>(ServerType::Hard)]);
        _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Pvp)] = CreateGroup<Field>(*this, _initData
            , ServerType::Pvp, _monitors[static_cast<std::vector<GroupID>::size_type>(ServerType::Pvp)]);
    }

    void Server::OnConnect(SessionID sessionId, const SockAddr_in& info)
    {
        //printf(format("Connect {:d} \n",sessionId.id).c_str());
    }

    void Server::OnDisconnect(SessionID sessionId, int wsaErrCode)
    {
        //printf(format("Disconnect {:d} \n",sessionId.id).c_str());

        {
            WRITE_LOCK;
            const auto it = _dbData.find(sessionId);
            if (it == _dbData.end())
            {
                return;
            }

            const auto& target = it->second;

            auto& conn = GetGameDbConnection();
            conn.Query("Update account set LoginState = 0 where (AccountNo = %d)", target->AccountNum());
            conn.reset();

            _dbData.erase(it);
        }
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
        if (_initData.consoleMonitor == false)
        {
            return;
        }
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

        PrintMonitorString();
        auto monitorStr = std::format(
            L"==================================================================================\n"
            L" {:<11s}{:^55s}{:>11s}\n"
            L"----------------------------------------------------------------------------------\n"
            , L"", L"Content", L"");

        for (const auto& [id, workTime, queued, jobTps,maxWork,dbErr,players,dbDeq,circle, square] : _monitors)
        {
            monitorStr.append(std::format(L"+  {:<7s} : {:>5d}, {:<7s} : {:>5}, {:<7s} : {:>5d}, {:<7s} : {:>5d}\n"
                                          , L"ID", id, L"WORK", workTime, L"DBErr", dbErr, L"MaxWork", maxWork));
            monitorStr.append(std::format(L"+  {:<7s} : {:>5d}, {:<7s} : {:>5d}, {:<7s} : {:>5d}, {:<7s} : {:>5d}\n"
                                          , L"QUEUED", queued, L"JobTps", jobTps, L"Players", players, L"DBDequeue"
                                          , dbDeq));
            monitorStr.append(L"----------------------------------------------------------------------------------\n");
        }

        wprintf(monitorStr.c_str());
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

    void Server::SendMonitorData(const en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, const int value)
    {
        auto buffer = SendBuffer::Alloc();
        buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << static_cast<WORD>(0) <<
            static_cast<char>(0) << type << value << static_cast<int>(time(nullptr));
        SendPacket(_monitorSession, buffer);
    }

    void Server::SendLogin()
    {
        auto buffer = SendBuffer::Alloc();
        buffer << en_PACKET_SS_MONITOR_LOGIN << static_cast<WORD>(0) << static_cast<char>(0);
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


        auto& conn = GetGameDbConnection();
        conn.Query("select AccountNo, ID, PASS,LoginState from account where ID = '%s'", cid.c_str());
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
        SendPacket(sessionId, loginResult);
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


        auto& conn = GetGameDbConnection();

        conn.Query("select Nick,HP,Coins,CharType,ServerType,LocationX,LocationY from mydb.player where AccountNo = %d"
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
        {
            WRITE_LOCK;
            if (auto [_,result] = _dbData.emplace(sessionId
                                                  , std::make_shared<DBData>(sessionId, AccountNo, location
                                                                             , serverType, charType, coins, hp, nick));
                result == false)
            {
                _connectionLogger.Write(L"DB", CLogger::LogLevel::Debug, L"Cannot create DBData. %d %d %s", AccountNo
                                        , serverType, nick.ToString());
                DisconnectSession(sessionId);
                //플레이어 생성에 실패한 관련 에러 처리.
            }
            else
            {
                conn.Query("Update account set LoginState = 1 where (AccountNo = %d)", AccountNo);
                conn.reset();
            }
        }

        if (hp <= 0)
        {
            //마을로 보낸다.
            _groupManager->MoveSession(
                sessionId, _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Village)]);
        }
        else
        {
            _groupManager->MoveSession(sessionId, _groups[static_cast<std::vector<GroupID>::size_type>(serverType)]);
        }
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
