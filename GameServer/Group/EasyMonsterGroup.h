#pragma once
#include "GroupCommon.h"

namespace psh
{
    class Item;
    class Monster;

    class EasyMonsterGroup : public psh::GroupCommon
    {
        static constexpr int MAX_MONSTER = 5;

    public:
        EasyMonsterGroup(Server& server,ServerType type, short mapSize = 6400, short sectorSize = 800);
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
    
    protected:
        void SendMonitor() override;

    public:
        ~EasyMonsterGroup() override;
        
    private:
        unique_ptr<GameMap<shared_ptr<Monster>>> _monsterMap;
        
        unique_ptr<GameMap<shared_ptr<Item>>> _itemMap;
    };
}
