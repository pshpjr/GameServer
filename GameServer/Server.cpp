#include "stdafx.h"
#include "Server.h"
#include "GameMap.h"
#include "LockGuard.h"
#include "Player.h"
#include "PacketGenerated.h"
#include "Group/VillageGroup.h"

namespace psh 
{
	Server::Server() : _groups(ServerType::End,GroupID::InvalidGroupID())
	{
		_groups[ServerType::Village] = CreateGroup<VillageGroup>(this);
	}

	void Server::OnConnect(SessionID sessionId, const SockAddr_in& info)
	{
		printf(format("Connect {:d} \n",sessionId.id).c_str());
	}

	void Server::OnDisconnect(SessionID sessionId)
	{
		printf(format("Disconnect {:d} \n",sessionId.id).c_str());
		auto it = g_players.find(sessionId);
		if(it == g_players.end())
			DebugBreak();
		
		auto target = it->second;
		g_players.erase(it);
	}



	void Server::OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer)
	{
		psh::ePacketType type;
		buffer >> type;

		switch (type)
		{
		case psh::ePacketType::None:
			DebugBreak();
			break;
		case psh::eLogin_ReqLogin:
			OnLoginLogin(sessionId, buffer);
			break;
		case psh::eLogin_ReqRegister:
			break;
		case psh::eGame_ReqLogin:
			OnLogin(sessionId, buffer);
			break;
		default:
			break;
		}
	}

	void Server::OnMonitorRun()
	{
		IOCP::OnMonitorRun();
	}

	shared_ptr<Player> Server::getPlayerPtr(SessionID id)
	{
		//없으면 알아서 터짐.

		READ_LOCK
		auto ret = g_players.find(id)->second;
		
		return ret;
	}

	//로그인 서버 겸용으로 쓰다가 나중에 제거. 
	void Server::OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer)
	{
		printf(format("Login to LoginServer {:d} \n",sessionId.id).c_str());
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
		printf(format("Login to GameServer {:d} \n",sessionId.id).c_str());
		using namespace psh;
		AccountNo playerID;
		SessionKey key;

		AccountNo ClientId = g_AccountNo++;
		{
			GetGame_ReqLogin(buffer, playerID, key);

			auto loginResult = SendBuffer::Alloc();

			MakeGame_ResLogin(loginResult,ClientId, true);
			SendPacket(sessionId, loginResult);
			printf(format("Send Game Login Success {:d} \n",sessionId.id).c_str());
		}


		
		//임시 스폰 테스트용 
		FVector location = { (float)(rand() % 100 + 1000), (float)(rand() % 100 + 1000),0 };
		FVector direction = { 1, 1,0 };
		FVector vector{0,0,0,};
		
		auto [it, result] = g_players.emplace(sessionId,make_shared<Player>(location,direction,vector,sessionId,ClientId));
		printf(format("CraetePlayer {:d} \n",ClientId).c_str());
		if(result == false)
		{
			//플레이어 생성에 실패한 관련 에러 처리. 
		}

		_groupManager->MoveSession(sessionId,_groups[ServerType::Village]);
	}


}
