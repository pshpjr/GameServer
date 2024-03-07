#pragma once
#include "GroupCommon.h"

namespace psh
{
    class Item;
    class Monster;

    class EasyMonsterGroup : public psh::GroupCommon
    {
        static constexpr int MAX_MONSTER = 1;
    public:
        EasyMonsterGroup(Server* server,short mapSize = 6400, short sectorSize = 800);
        void OnEnter(SessionID id) override;
        void OnLeave(SessionID id) override;
        void OnChangeComp(SessionID id, CRecvBuffer& recvBuffer);
        void OnReqLevelChange(SessionID id, CRecvBuffer& recvBuffer) const;
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        bool SpawnItem(const shared_ptr<GameObject>& object);
        void OnActorDestroy(const shared_ptr<GameObject>& object) override;
        void GetActors(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets);
        void GetDelActors(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets);
        
    protected:
        void UpdateContent(float delta) override;
        void SendMonitor() override;

    public:
        void CheckVictim(const Range& attackRange, int damage, const shared_ptr<ChatCharacter>& attacker) override;
        void CheckItem(const shared_ptr<ChatCharacter>& target) override;
        ~EasyMonsterGroup() override;
        void BroadcastMove(const shared_ptr<ChatCharacter>& player, FVector oldLocation, FVector newLocation) override;

    private:
        void OnMove(SessionID sessionId,CRecvBuffer& buffer);
        void OnAttack(SessionID sessionId, CRecvBuffer& buffer);

        ObjectID g_objectId = 0;

        ObjectID GetNextID(){return g_objectId++;}
        void SpawnMonster();
        //content
        SessionMap<shared_ptr<Player>> _players;
        HashMap<ObjectID,shared_ptr<Monster>> _monsters;
        HashMap<ObjectID,shared_ptr<Item>> _items;
        
        unique_ptr<GameMap<ChatCharacter>> _monsterMap;
        unique_ptr<GameMap<Item>> _itemMap;
    };
}
