#pragma once
#include "GroupCommon.h"

namespace psh
{
    class Monster;

    class EasyMonsterGroup : public psh::GroupCommon
    {
    public:
        EasyMonsterGroup(Server* server,short mapSize = 6400, short sectorSize = 800);
        void OnEnter(SessionID id) override;
        void OnLeave(SessionID id) override;
        void OnChangeComp(SessionID id, CRecvBuffer& recvBuffer);
        void OnReqLevelChange(SessionID id, CRecvBuffer& recvBuffer) const;
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;

    protected:
        void UpdateContent(float delta) override;
        void SendMonitor() override;

    public:
        ~EasyMonsterGroup() override;
    private:
        void OnMove(SessionID sessionId,CRecvBuffer& buffer);
        void OnAttack(SessionID sessionId, CRecvBuffer& buffer);

        //content
        SessionMap<shared_ptr<Player>> _players;
        
        vector<Monster> monsters;
        unique_ptr<GameMap> _monsterMap;
    };
}
