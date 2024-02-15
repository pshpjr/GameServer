#pragma once
#include "ContentTypes.h"
#include "GroupCommon.h"

namespace psh
{
    class GameMap;

    class VillageGroup : public GroupCommon
    {
    public:
        void OnEnter(SessionID id) override;
        void OnLeave(SessionID id) override;
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;

    protected:
        void UpdateContent(int delta) override;
        void SendMonitor() override;

    public:
        ~VillageGroup() override;
    private:
        void OnLoginLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnLogin(SessionID sessionId, CRecvBuffer& buffer);
        void OnMove(SessionID sessionId,CRecvBuffer& buffer);
    private:
        psh::AccountNo g_AccountNo = 0;
        SessionMap<shared_ptr<Player>> _players;
        unique_ptr<GameMap> _gameMap;
    };

}

