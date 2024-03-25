#include "VillageGroup.h"
#include "ObjectManager.h"
VillageGroup::VillageGroup(psh::Server& server,const ServerInitData& data, short mapSize, short sectorSize)
: GroupCommon(server,data,psh::ServerType::Village,mapSize,sectorSize)
{
    _objectManager = make_unique<psh::ObjectManager>(*this,*_playerMap);
}