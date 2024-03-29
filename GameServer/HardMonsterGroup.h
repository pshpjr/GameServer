#pragma once
#include "GroupCommon.h"

namespace psh
{
    class Item;
    class Monster;
    class HardMonsterGroup :
        public GroupCommon
    {
        static constexpr int MAX_MONSTER = 30;

    public:
        HardMonsterGroup(Server& server, const ServerInitData& data, ServerType type, short mapSize = 6400, short sectorSize = 400);
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        void UpdateContent(int deltaMs) override;
    protected:

    public:
        ~HardMonsterGroup() override;

    private:
        float inputDelay = 0;
        unique_ptr<GameMap<shared_ptr<Monster>>> _monsterMap;

        unique_ptr<GameMap<shared_ptr<Item>>> _itemMap;
    };

}

