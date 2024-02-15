#pragma once
#include <shared_mutex>
#include <vector>
#include <functional>

#include "ContentTypes.h"
#include "flat_unordered_set.h"

namespace psh
{
    class Server;
    struct FVector;
    class Player;
    struct Sector;
    class GameMap 
    {
        const short SECTOR_SIZE = 100;
        const short MAP_SIZE = 0;
        const short MAX_SECTOR_INDEX_X = MAP_SIZE / SECTOR_SIZE;
        const short MAX_SECTOR_INDEX_Y = MAP_SIZE / SECTOR_SIZE;
    
        class LockSector : public flat_unordered_set<shared_ptr<psh::Player>>
        {
        public:
            auto begin() {
                return flat_unordered_set<shared_ptr<psh::Player>>::begin();
            }

            auto end() {
                return flat_unordered_set<shared_ptr<psh::Player>>::end();
            }
            unique_ptr<shared_mutex> lock;
        };
    public:

        GameMap(const int mapSize,const int sectorSize, Server* owner);
        ~GameMap() = default;
        
        GameMap(const GameMap& other) = delete;
        GameMap(GameMap&& other) noexcept = delete;
        GameMap& operator=(const GameMap& other) = delete;
        GameMap& operator=(GameMap&& other) noexcept = delete;
        void PrintPlayerInfo();
        
        vector<vector<int>> GetPlayerInfo();
        
        void AddPlayer(shared_ptr<Player>& target);
        void BroadcastNewPlayerCreated(shared_ptr<Player>& target);
        void BroadcastAttack(shared_ptr<Player>& target);
        void RemovePlayer(shared_ptr<Player>& target);
        
        void MovePlayer(shared_ptr<Player>& target, const FVector newLocation);
        
        auto GetSectorsViewFromOffset(const Sector& targetSector, std::span<const pair<short,short>> offsets);

    private:
        [[nodiscard]] Sector GetSector(const FVector location) const;
        
        void AddPlayer(Sector targetSector, shared_ptr<Player>& target);
        void RemovePlayer(Sector curSector, shared_ptr<Player>& target);

        bool IsValidSector(const Sector sector) const;
        void MovePlayer(psh::FVector oldLocation, psh::FVector newLocation, shared_ptr<Player>& target);

        void Iterate(FVector radius , const function<void(Player&)>& toInvoke);
        void Iterate( psh::FVector start, psh::FVector end , const function<void(Player&)>& toInvoke);
        
        void ApplyToPlayerInSector(Sector target, const std::function<void(Player&)>& toInvoke);
        
        void AcquireExclusiveLock(const Sector targetSector);
        void ReleaseExclusiveLock(const Sector targetSector);
        void AcquireSharedLock(const Sector targetSector);
        void ReleaseSharedLock(const Sector targetSector);

  
    private:
        std::vector < std::vector <LockSector>> _map;

        Server* _owner;
    };

}
