#pragma once
#include "GroupCommon.h"

namespace psh
{
    class Item;
    class Monster;

    class EasyMonsterGroup : public psh::GroupCommon
    {
        static constexpr int MAX_MONSTER = 0;

    public:
        EasyMonsterGroup(Server& server, const ServerInitData& data,ServerType type, short mapSize = 6400, short sectorSize = 400);
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        void UpdateContent(int deltaMs) override;
    protected:
        void SendMonitor() override;

    public:
        ~EasyMonsterGroup() override;
        
    private:
        float inputDelay = 0;
        unique_ptr<GameMap<shared_ptr<Monster>>> _monsterMap;
        
        unique_ptr<GameMap<shared_ptr<Item>>> _itemMap;
    };
}
