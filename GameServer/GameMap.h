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
    
        
    public:

        GameMap(const short mapSize,const short sectorSize, Server* owner, ServerType type);
        ~GameMap() = default;
        
        GameMap(const GameMap& other) = delete;
        GameMap(GameMap&& other) noexcept = delete;
        GameMap& operator=(const GameMap& other) = delete;
        GameMap& operator=(GameMap&& other) noexcept = delete;
        
        //새로 플레이어 생성할 때
        void SpawnPlayer(shared_ptr<Player>& target, ServerType type);
        void DestroyPlayer(shared_ptr<Player>& target);
        
        void MovePlayer(shared_ptr<Player>& target, const FVector newLocation);
        void BroadcastAttack(shared_ptr<Player>& target);
        void BroadcastHit(shared_ptr<Player>& target);
        void BroadcastMoveStart(shared_ptr<Player>& target, const FVector newLocation);
        
        void PrintPlayerInfo();
        
        vector<vector<int>> GetPlayerInfo();

    private:
        std::span<const pair<short, short>> GetMoveOffset(Sector moveInfo);
        void AddPlayer(shared_ptr<Player>& target);
        void RemovePlayer(shared_ptr<Player>& target);

        //해당 플레이어에게는 주변 플레이어 정보를, 주변 플레이어에게는 해당 플레이어 정보를 전송한다. 
        void SendCreateCharacter(shared_ptr<Player>& target, bool isSpawn , std::span<const pair<short, short>> offsets);
        void SendRemoveCharacter(shared_ptr<Player>& target, bool isDead , std::span<const pair<short, short>> offsets);
        
       void SendToSectors(const Sector& targetSector, SendBuffer& buffer, std::span<const pair<short, short>> offsets);
        auto GetSectorsViewFromOffset(const Sector& targetSector, std::span<const pair<short,short>> offsets);

        
        [[nodiscard]] Sector GetSector(const FVector location) const;
        
        void AddPlayer(Sector targetSector, shared_ptr<Player>& target);
        void RemovePlayer(Sector curSector, shared_ptr<Player>& target);

        bool IsValidSector(const Sector sector) const;
 
        void Iterate(FVector radius , const function<void(Player&)>& toInvoke);
        void Iterate( psh::FVector start, psh::FVector end , const function<void(Player&)>& toInvoke);
        
        void ApplyToPlayerInSector(Sector target, const std::function<void(Player&)>& toInvoke);

        void UpdatePlayer();
    
    private:
        using playerSecor = flat_unordered_set<shared_ptr<psh::Player>>;
        std::vector < std::vector <playerSecor>> _map;
        
        Server* _owner;
    };

}
