#pragma once
#include <shared_mutex>
#include <vector>
#include <functional>

#include "ContentTypes.h"
#include "flat_unordered_set.h"
#include "Data/Item.h"

namespace psh
{
    class ChatCharacter;
    struct Range;
    class Server;
    struct FVector;
    class Player;
    struct Sector;

    //언리얼의 좌표계는 y가 오른쪽. 좌하단이 0, 앞이 x
    class GameMap 
    {
        const short SECTOR_SIZE = 100;
        const short MAP_SIZE = 0;
        const short MAX_SECTOR_INDEX_X = MAP_SIZE / SECTOR_SIZE;
        const short MAX_SECTOR_INDEX_Y = MAP_SIZE / SECTOR_SIZE;

        const short GRID_SIZE = 100;
        
    public:
        GameMap(short mapSize, short sectorSize, Server* owner);
        ~GameMap() = default;
        
        



        //새로 플레이어 생성할 때
        void SpawnPlayer(shared_ptr<Player>& target);
        
        void CheckVictim(const Range& attackRange, int damage,const shared_ptr<ChatCharacter>& attacker);
        void BroadcastDisconnect(const shared_ptr<Player>& target);
        void BroadcastIfSectorChange(const shared_ptr<ChatCharacter>& target, FVector oldLocation, psh::FVector newLocation);
        void Broadcast(FVector location,SendBuffer& buffer);
        
        void PrintPlayerInfo();
        
        vector<vector<int>> GetPlayerInfo();
        int Size() const {return MAP_SIZE;}
        int Players();
        [[nodiscard]] Sector GetSector(FVector location) const;
    private:
        GameMap(const GameMap& other) = delete;
        GameMap(GameMap&& other) noexcept = delete;
        GameMap& operator=(const GameMap& other) = delete;
        GameMap& operator=(GameMap&& other) noexcept = delete;


        //해당 플레이어에게는 주변 플레이어 정보를, 주변 플레이어에게는 해당 플레이어 정보를 전송한다. 
        void SendCreateCharacterAndGetInfo(const shared_ptr<Player>& target);

        void SendToSectors(const Sector& targetSector,SendBuffer& buffer, std::span<const psh::Sector> offsets, const shared_ptr<psh::
                              Player>& exclude);
        

        void AddPlayer(const shared_ptr<Player>& target);
        void AddPlayer(Sector targetSector, const shared_ptr<Player>& target);
        
        void RemovePlayer(const shared_ptr<Player>& target);
        void RemovePlayer(Sector curSector, const shared_ptr<Player>& target);
        
        void ApplyToPlayerInSector(Sector target, std::span<const psh::Sector> offsets, const std::function<void(shared_ptr<Player>&)>& toInvoke);


        bool IsValidSector(Sector sector) const;
        psh::FVector GetRandomLocation() const ;

        auto GetSituatedSectorView(const psh::Sector& target, std::span<const psh::Sector> offsets);
        auto GetSectorsFromOffset(const psh::Sector& target, std::span<const psh::Sector> offsets) const ;
        auto GetVicinalSectorView(const FVector& p1, const FVector& p2);
        auto GetSectorsFromRange(const FVector& point1, const FVector& point2) const;
        auto GetValidSectorView();
        auto GetPlayersFromView();
        auto ToMapReference() const;
        auto GetValidSectors() const;

    private:
        using playerSector = flat_unordered_set<shared_ptr<psh::Player>>;
        std::vector < std::vector <playerSector>> _map;
        std::vector< std::vector<Item>> _grid;
        
        Server* _owner;
    };

}
