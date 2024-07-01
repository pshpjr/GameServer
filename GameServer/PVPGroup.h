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
        //디버깅 내용 추가용. pvp에서만 확인할 게 있음. 
        //ResAttack과 동일. 
        void PvpResAttack(SessionID sessionId, CRecvBuffer& buffer); 
        void PvpSendInRange(FVector location
            , std::span<const Sector> offsets
            , SendBuffer& buffer
            , const shared_ptr<psh::Player>& exclude = nullptr);
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
