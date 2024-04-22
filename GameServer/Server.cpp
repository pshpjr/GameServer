#include "stdafx.h"
#include "Server.h"

#include "DBConnection.h"
#include "GameMap.h"
#include "LockGuard.h"
#include "Player.h"
#include "PacketGenerated.h"
#include "ObjectManager.h"
#include "EasyMonsterGroup.h"
#include "DBData.h"
#include "MockData.h"
#include "PVPGroup.h"
#include "HardMonsterGroup.h"

namespace psh
{
    Server::Server() : _groups(static_cast<vector<GroupID>::size_type>(ServerType::End), GroupID::InvalidGroupID()),
    DBTLSId(TlsAlloc())
    {

        serverSettings.Init(L"GameSettings.txt");
        serverSettings.GetValue(L"db.GameDBIP",_initData.gameDBIP);
        serverSettings.GetValue(L"db.GameDBPort",_initData.gameDBPort);
        serverSettings.GetValue(L"db.GameDBID",_initData.gameDBID);
        serverSettings.GetValue(L"db.GameDBPwd",_initData.gameDBPwd);
        serverSettings.GetValue(L"db.MonitorIP",_initData.MonitorServerIP);
        serverSettings.GetValue(L"db.MonitorPort",_initData.MonitorServerPort);
        
        serverSettings.GetValue(L"db.useMonitorServer", _initData.UseMonitorServer);

        _groups[static_cast<vector<GroupID>::size_type>(ServerType::Village)] = CreateGroup<GroupCommon>(*this,_initData, ServerType::Village);
        _groups[static_cast<vector<GroupID>::size_type>(ServerType::Easy)] = CreateGroup<EasyMonsterGroup>(*this,_initData,ServerType::Easy);
        _groups[static_cast<vector<GroupID>::size_type>(ServerType::Hard)] = CreateGroup<HardMonsterGroup>(*this,_initData,ServerType::Hard);
        _groups[static_cast<vector<GroupID>::size_type>(ServerType::Pvp)] = CreateGroup<PvpGroup>(*this,_initData,ServerType::Pvp);
    }

    void Server::OnConnect(const SessionID sessionId, const SockAddr_in& info)
    {
        //printf(format("Connect {:d} \n",sessionId.id).c_str());
    }
    
    void Server::OnDisconnect(const SessionID sessionId)
    {
        //printf(format("Disconnect {:d} \n",sessionId.id).c_str());

        {
            WRITE_LOCK
            const auto it = g_dbData.find(sessionId);
            if (it == g_dbData.end())
            {
                return;
            }
            auto& target = it->second;
            g_dbData.erase(it);
        }
    }

    void Server::OnRecvPacket(const SessionID sessionId, CRecvBuffer& buffer)
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
        //PrintMonitorString();
        //printf("g_dbDataSize : %lld\n", g_dbData.size());
        if(GetAsyncKeyState(VK_HOME))
        {
        	Stop();
        }
    }

    shared_ptr<DBData> Server::getDbData(const SessionID id)
    {
        //없으면 알아서 터짐.

        READ_LOCK
        auto& ret = g_dbData.find(id)->second;

        return ret;
    }

    //로그인 서버 겸용으로 쓰다가 나중에 제거. 
    void Server::OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer)
    {
        //printf(format("Login to LoginServer {:d} \n", sessionId.id).c_str());
        using namespace psh;
        static AccountNo gAccountNo = 0;

        ID playerID;
        Password playerPass;
        GetLogin_ReqLogin(buffer, playerID, playerPass);
        
        //auto conn = GetGameDbConnection();
        //conn->Query("select AccountNo, ID, PASS from account where ID = \"%s\"", playerID);
        //auto loginResult = SendBuffer::Alloc();

        //if (!conn->next()) 
        //{
        //    MakeLogin_ResLogin(loginResult, gAccountNo++, playerID, eLoginResult::InvalidId, SessionKey());
        //}
        //else
        //{
        //    AccountNo accountNo = conn->getInt(0);
        //    ID trueID(conn->getString(1));
        //    Password truePass(conn->getString(2));

        //}

        auto loginResult = SendBuffer::Alloc();
        MakeLogin_ResLogin(loginResult, gAccountNo++, playerID, eLoginResult::LoginSuccess, SessionKey());
        SendPacket(sessionId, loginResult);
    }

    void Server::OnLogin(SessionID sessionId, CRecvBuffer& buffer)
    {
        //printf(format("Login to GameServer {:d} \n", sessionId.id).c_str());
        using namespace psh;
        AccountNo AccountNo;
        SessionKey key;
        
        {
            //로그인은 무조건 성공
            GetGame_ReqLogin(buffer, AccountNo, key);


            auto loginResult = SendBuffer::Alloc();

            MakeGame_ResLogin(loginResult, AccountNo, true);
            SendPacket(sessionId, loginResult);
        }

        //일단 AccounNo가 특정 범위이면 하는걸로
        if(AccountNo <= 1000)
        {
            auto conn = GetGameDbConnection();

            conn->Query("select Nick,HP,Coins,CharType,ServerType,LocationX,LocationY,LoginState from mydb.player where AccountNo = %d",AccountNo);

            if ( !conn->next() )
            {
                // InterlockedIncrement(&_dbErrorCount);
                // gLogger->Write(L"Redis", LogLevel::Debug, L"DBFail : %d ", AccountNo);
                //
                // sendFail(AccountNo, sessionId, L"LoginFailRelease",*connectionResult);
                DisconnectSession(sessionId);
                return;
            }
            
            Nickname nick(conn->getString(0));
            int hp = conn->getInt(1);
            int coins = conn->getInt(2);
            char charType = conn->getChar(3);
            char serverType = conn->getChar(4);
            FVector location = {conn->getFloat(5),conn->getFloat(6)};
            bool loginState = conn->getChar(7);
            
            conn->reset();
            {
                WRITE_LOCK
                auto [_,result] = g_dbData.emplace(sessionId
                    , make_shared<DBData>(sessionId, AccountNo, location , serverType, charType, coins, hp, nick));
                if (result == false)
                {
                    //플레이어 생성에 실패한 관련 에러 처리. 
                }
            }

            if(hp <=0)
            {
                //마을로 보낸다.
                _groupManager->MoveSession(sessionId, _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Village)]);
            }
            else
            {
                _groupManager->MoveSession(sessionId, _groups[static_cast<std::vector<GroupID>::size_type>(serverType)]);
            }
        }

    }

    DBConnection* Server::GetGameDbConnection()
    {        
        void* tlsValue = TlsGetValue(DBTLSId);

        if ( tlsValue == nullptr )
        {
            {
                WRITE_LOCK_IDX(1)
                tlsValue = (void* )new DBConnection(_initData.gameDBIP.c_str(),_initData.gameDBPort, _initData.gameDBID.c_str(), _initData.gameDBPwd.c_str(), "mydb");
            }
            TlsSetValue(DBTLSId, tlsValue);
        }
        return ( DBConnection* ) tlsValue;
    }

    DBConnection& Server::GetGameDbConnection()
    {        
        auto tlsValue = TlsGetValue(DBTLSId);

        if ( tlsValue == nullptr )
        {
            {
                WRITE_LOCK_IDX(1)
                tlsValue = new DBConnection(_initData.gameDBIP.c_str(),_initData.gameDBPort, _initData.gameDBID.c_str(), _initData.gameDBPwd.c_str(), "mydb");
            }
            TlsSetValue(DBTLSId, tlsValue);
        }
        return *( DBConnection* ) tlsValue;
    }
}
