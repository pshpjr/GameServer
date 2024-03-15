#include "VillageGroup.h"
#include "../Base/ObjectManager.h"
VillageGroup::VillageGroup(psh::Server& server, short mapSize, short sectorSize)
: GroupCommon(server,psh::ServerType::Village,mapSize,sectorSize)
{
    _objectManager = make_unique<psh::ObjectManager>(*this,*_playerMap);
}