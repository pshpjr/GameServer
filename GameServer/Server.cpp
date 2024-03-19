#include "stdafx.h"
#include "Server.h"
#include "GameMap.h"
#include "LockGuard.h"
#include "Base/Player.h"
#include "PacketGenerated.h"
#include "Base/ObjectManager.h"
#include "Group/EasyMonsterGroup.h"
#include "Data/DBData.h"
#include "Data/MockData.h"

namespace psh
{
    Server::Server() : _groups(static_cast<vector<GroupID>::size_type>(ServerType::End), GroupID::InvalidGroupID())
    {
        _groups[static_cast<vector<GroupID>::size_type>(ServerType::Village)] = CreateGroup<GroupCommon>(*this,ServerType::Village);
        _groups[static_cast<vector<GroupID>::size_type>(ServerType::Easy)] = CreateGroup<EasyMonsterGroup>(*this,ServerType::Easy);
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
            const auto it = g_players.find(sessionId);
            if (it == g_players.end())
            {
                return;
            }
            auto target = it->second;
            g_players.erase(it);
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
                break;
        }
    }

    void Server::OnMonitorRun()
    {
        PrintMonitorString();

        // if(GetAsyncKeyState('D'))
        // {
        // 	Stop();
        // }
    }

    shared_ptr<DBData> Server::getDbData(const SessionID id)
    {
        //없으면 알아서 터짐.

        READ_LOCK
        auto ret = g_players.find(id)->second;

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
            GetGame_ReqLogin(buffer, AccountNo, key);

            auto loginResult = SendBuffer::Alloc();

            MakeGame_ResLogin(loginResult, AccountNo, true);
            SendPacket(sessionId, loginResult);
            //printf(format("Send Game Login Success {:d} \n", sessionId.id).c_str());
        }

        

        {
            FVector location = FVector(RandomUtil::Rand(0,6300), RandomUtil::Rand(0,6300));
            //FVector location = {3000, 3000};
            WRITE_LOCK
            auto nickIndex = rand() % gNicks.size();
            auto [it, result] = g_players.emplace(sessionId
                                                  , make_shared<DBData>(sessionId, AccountNo, location, 1, rand() % 4, 0
                                                                        , 100,gNicks[nickIndex]));
            //printf(format("CreatePlayer {:d} \n", AccountNo).c_str());
            if (result == false)
            {
                //플레이어 생성에 실패한 관련 에러 처리. 
            }
        }
        _groupManager->MoveSession(sessionId, _groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Easy)]);
    }
}
