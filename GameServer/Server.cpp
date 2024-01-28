#include "stdafx.h"
#include "Server.h"
#include "PacketGenerated.h"

namespace psh 
{
	Server::Server()
	{
	}

	void Server::OnConnect(SessionID sessionId, const SockAddr_in& info)
	{
	}

	void Server::OnDisconnect(SessionID sessionId)
	{
	}

	void Server::OnRecvPacket(SessionID sessionId, CRecvBuffer& buffer)
	{
		psh::ePacketType type;
		buffer >> type;

		switch (type)
		{
		case psh::None:
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
	}

	//로그인 서버 겸용으로 쓰다가 나중에 제거. 
	void Server::OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer)
	{
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
		using namespace psh;
		AccountNo playerID;
		SessionKey key;

		{
			GetGame_ReqLogin(buffer, playerID, key);

			auto loginResult = SendBuffer::Alloc();

			MakeGame_ResLogin(loginResult, playerID, true);
			SendPacket(sessionId, loginResult);
		}

		//임시 스폰 테스트용 
		FVector location = { 500,500,0 };
		FVector direction = { 0.5, 0.5,0 };

		//FVector location = { rand() % 6400, rand() % 6400,0 };
		//FVector direction = { rand() % 1, rand() % 1,0 };

		auto spawnPlayer = SendBuffer::Alloc();

		MakeGame_ResCreateUser(spawnPlayer, playerID, 1, 1, location, direction);
		SendPacket(sessionId, spawnPlayer);
	}

}
