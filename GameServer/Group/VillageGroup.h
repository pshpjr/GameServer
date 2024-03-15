#pragma once
#include "GroupCommon.h"

class VillageGroup : public psh::GroupCommon
{
protected:
    VillageGroup(psh::Server& server, short mapSize = 6400, short sectorSize = 800);
    void SendMonitor() override{}

public:
    void RecvAttack(SessionID sessionId, CRecvBuffer& buffer) override{}
};
