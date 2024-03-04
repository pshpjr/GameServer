#include "stdafx.h"
#include "Server.h"
#include "GameMap.h"
#include "LockGuard.h"
#include "Base/Player.h"
#include "PacketGenerated.h"
#include "Group/EasyMonsterGroup.h"
#include "Group/VillageGroup.h"


namespace psh 
{
	Server::Server() : _groups(static_cast<vector<GroupID>::size_type>(ServerType::End),GroupID::InvalidGroupID())
	{
		_groups[static_cast<vector<GroupID>::size_type>(ServerType::Village)] = CreateGroup<VillageGroup>(this);
		_groups[static_cast<vector<GroupID>::size_type>(ServerType::Easy)] = CreateGroup<EasyMonsterGroup>(this);
	}

	void Server::OnConnect(const SessionID sessionId, const SockAddr_in& info)
	{
		_connects.insert(sessionId);	
		//printf(format("Connect {:d} \n",sessionId.id).c_str());
	}

	//TODO: 멀티스레드 처리
	void Server::OnDisconnect(const SessionID sessionId)
	{
		//printf(format("Disconnect {:d} \n",sessionId.id).c_str());
		
		{
			WRITE_LOCK
			const auto it = g_players.find(sessionId);
			 if(it == g_players.end())
				return;
			auto target = it->second;
			g_players.erase(it);
		}


	}
	
	void Server::OnRecvPacket(const SessionID sessionId, CRecvBuffer& buffer)
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

		//PrintMonitorString();

		if(GetAsyncKeyState('D'))
		{
			Stop();
		}

	}

	shared_ptr<Player> Server::getPlayerPtr(const SessionID id)
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
		AccountNo AccountNo;
		SessionKey key;


		{
			GetGame_ReqLogin(buffer, AccountNo, key);

			auto loginResult = SendBuffer::Alloc();

			MakeGame_ResLogin(loginResult, AccountNo, true);
			SendPacket(sessionId, loginResult);
			printf(format("Send Game Login Success {:d} \n",sessionId.id).c_str());
		}

		
		//임시 스폰 테스트용 
		FVector location = { (float)(rand() % 400 + 50), (float)(rand() % 400 + 50)};
		FVector direction = { 0, 0 };
		ObjectID clientId = g_clientID++;
		//TODO: 플레이어 위치는 각 그룹에서 정해야 함. 
		{
			WRITE_LOCK
			auto [it, result] = g_players.emplace(sessionId,make_shared<Player>(clientId,location,direction,rand()%4,sessionId,AccountNo));
			printf(format("CreatePlayer {:d} \n", AccountNo).c_str());
			if(result == false)
			{
				//플레이어 생성에 실패한 관련 에러 처리. 
			}
		}
		_groupManager->MoveSession(sessionId,_groups[static_cast<std::vector<GroupID>::size_type>(ServerType::Easy)]);
	}


}
