#pragma once
#include "ContentTypes.h"
#include "GroupCommon.h"

namespace psh
{
    class VillageGroup : public GroupCommon
    {
    public:
        VillageGroup(Server* server,short mapSize = 6400, short sectorSize = 800) :
        GroupCommon(server, ServerType::Village,mapSize,sectorSize){}
        void OnEnter(SessionID id) override;
        void OnLeave(SessionID id) override;
        void OnChangeComp(SessionID id, CRecvBuffer& recvBuffer);
        void OnReqLevelChange(SessionID id, CRecvBuffer& recvBuffer) const;
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        void CheckVictim(const Range& attackRange, int damage, const shared_ptr<ChatCharacter>& attacker) override;

    protected:
        template <typename T>
        void GetInfo(vector<SendBuffer>& buffer, FVector location, GameMap<T>* map, std::span<const psh::Sector> offsets);
        
        void UpdateContent(float delta) override;
        void SendMonitor() override;

    public:
        ~VillageGroup() override;
    private:
        void OnMove(SessionID sessionId,CRecvBuffer& buffer);
        void OnAttack(SessionID sessionId, CRecvBuffer& buffer);
        //content
        SessionMap<shared_ptr<Player>> _players;

    };

    template <typename T>
    void VillageGroup::GetInfo(vector<SendBuffer>& buffer, FVector location, GameMap<T>* map, std::span<const psh::Sector> offsets)
    {
        auto targets = map->GetSectorsFromOffset(map->GetSector(location),offsets);

        ranges::for_each(targets,[&buffer](decltype(targets) sectors)
        {
            for(auto& target : sectors)
            {
                target->GetInfo(buffer);
            }
        });
        
    }
}

