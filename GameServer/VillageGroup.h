#pragma once
#include "GroupCommon.h"

class VillageGroup : public psh::GroupCommon
{
protected:
    VillageGroup(psh::Server& server,const ServerInitData& data, short mapSize = 6400, short sectorSize = 800);

public:
    void RecvAttack(SessionID sessionId, CRecvBuffer& buffer) override{}
};
