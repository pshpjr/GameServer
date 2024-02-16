#pragma once
#include "ContentTypes.h"
#include "GroupCommon.h"

namespace psh
{
    class VillageGroup : public GroupCommon
    {
    public:
        VillageGroup(Server* server) : GroupCommon(server,ServerType::Village,3200,100){}
        void OnEnter(SessionID id) override;
        void OnLeave(SessionID id) override;
        void OnChangeComp(SessionID id, CRecvBuffer& recvBuffer);
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        
    protected:
        void UpdateContent(int delta) override;
        void SendMonitor() override;

    public:
        ~VillageGroup() override;
    private:
        void OnMove(SessionID sessionId,CRecvBuffer& buffer);
        void OnAttack(SessionID sessionId, CRecvBuffer& buffer);
    };

}

