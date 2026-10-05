#include "ChannelManager.h"

#include "Field.h"
#include "GroupManager.h"
#include "Rand.h"
#include "Server.h"

#include <fmt/base.h>

psh::ChannelManager::ChannelManager(Server* server, GroupManager* manager, ServerInitData data)
	: _groupManager{
		  manager
	  }, _server{server}, _initData{data}
{
	_groups[ServerType::Village].push_back(_groupManager->CreateGroup<Field>(*_server, _initData, ServerType::Village));
	_groups[ServerType::Easy].push_back(_groupManager->CreateGroup<Field>(*_server, _initData, ServerType::Easy));
	_groups[ServerType::Hard].push_back(_groupManager->CreateGroup<Field>(*_server, _initData, ServerType::Hard));
	_groups[ServerType::Pvp].push_back(_groupManager->CreateGroup<Field>(*_server, _initData, ServerType::Pvp));
}

psh::ChannelManager::~ChannelManager() = default;

GroupID psh::ChannelManager::GetGroupIDByType(ServerType type)
{
	auto index = psh::RandomUtil::Rand(0, _groups[type].size() - 1);
	return _groups[type][index];
}

void psh::ChannelManager::PrintChanalsState()
{
    fmt::print("==================================================================================\n"
               " {:<11s}{:^55s}{:>11s}\n"
               "----------------------------------------------------------------------------------\n",
               "", "Content", "");
}
