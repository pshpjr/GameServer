#include "stdafx.h"
#include "Server.h"
#include "GameMap.h"
#include "Player.h"
#include "PacketGenerated.h"

namespace psh 
{
	Server::Server() : _gameMap(make_unique<GameMap>(5000,5000,this))
	{
	}

	void Server::OnConnect(SessionID sessionId, const SockAddr_in& info)
	{
		printf(format("Connect {:d} \n",sessionId.id).c_str());
	}

	void Server::OnDisconnect(SessionID sessionId)
	{
		printf(format("Disconnect {:d} \n",sessionId.id).c_str());
		auto it = _players.find(sessionId);
		if(it == _players.end())
			return;
		
		auto target = it->second;
		_players.erase(it);
		_gameMap->RemovePlayer(target);
	}

	void Server::OnAttack(SessionID sessionId, CRecvBuffer& buffer)
	{
		auto player = _players[sessionId];
		AccountNo account;
		GetGame_ReqAttack(buffer,account);
		
		if(player == nullptr)
		{
			DisconnectSession(sessionId);
		}

		_gameMap->BroadcastAttack(player);
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
		case psh::eGame_ReqMove:
			OnMove(sessionId,buffer);
			break;
		case psh::eGame_ReqAttack:
			OnAttack(sessionId,buffer);
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
		
		auto [it, result] = _players.emplace(sessionId,make_shared<Player>(location,direction,vector,sessionId,ClientId));
		printf(format("CraetePlayer {:d} \n",ClientId).c_str());
		if(result == false)
		{
			//플레이어 생성에 실패한 관련 에러 처리. 
		}

		_gameMap->AddPlayer(it->second);
		//_gameMap->PrintPlayerInfo();
		_gameMap->BroadcastNewPlayerCreated(it->second);
		//FVector _location = { rand() % 6400, rand() % 6400,0 };
		//FVector _direction = { rand() % 1, rand() % 1,0 };
	}

	void Server::OnMove(SessionID sessionId, CRecvBuffer& buffer)
	{
		printf(format("Request Move {:d} \n",sessionId.id).c_str());
		AccountNo accountNo;
		FVector location;
		GetGame_ReqMove(buffer,accountNo,location);

		auto result = _players[sessionId];
		if(result == nullptr)
		{
			DisconnectSession(sessionId);
		}
		
		_gameMap->MovePlayer(result,location);
	}
}
