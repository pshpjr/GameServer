/* 
ChannelManager
Created by: pshpjr
Date: 2025-02-05

Description: 

*/

#pragma once

//STL 헤더

//서드파티 헤더

//프로젝트 헤더
#include "./Common/ContentTypes.h"
#include "ServerInitData.h"
class GroupManager;

namespace psh
{
	class Server;

	class ChannelManager
	{
	public:
		ChannelManager(Server* server, GroupManager* manager, ServerInitData data);
		~ChannelManager();

		GroupID GetGroupIDByType(ServerType type);
		void PrintChanalsState();

	private:
		GroupManager* _groupManager = nullptr;
		Server* _server;
		std::unordered_map<ServerType, std::vector<GroupID>> _groups{};
		ServerInitData _initData;
		// Private members go here
	};
}
