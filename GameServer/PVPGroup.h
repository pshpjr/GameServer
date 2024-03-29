#pragma once
#include "GroupCommon.h"

namespace psh
{
    class Item;
    class Monster;
    class PvpGroup :
        public GroupCommon
    {
        static constexpr int MAX_MONSTER = 30;

    public:
        PvpGroup(Server& server, const ServerInitData& data, ServerType type, short mapSize = 6400, short sectorSize = 400);
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        void UpdateContent(int deltaMs) override;
        void SendMonitor() override; 
    protected:

    public:
        ~PvpGroup() override;

        struct Debug 
        {
            int revChange = 0;
            int move = 0;
            int attack = 0;
            int reqLevelChange = 0;
        }debugData;

    private:
        float inputDelay = 0;
        unique_ptr<GameMap<shared_ptr<Monster>>> _monsterMap;

        unique_ptr<GameMap<shared_ptr<Item>>> _itemMap;
    };

};
