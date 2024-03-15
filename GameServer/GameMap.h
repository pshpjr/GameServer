#pragma once

#include <iostream>
#include <vector>
#include <random>
#include "Sector.h"
#include "flat_unordered_set.h"
#include "Base/Rand.h"
#include "Base/Item.h"

namespace psh
{
    class GameObject;
    struct Range;
    class Server;
    struct FVector;

    //언리얼의 좌표계는 y가 오른쪽. 좌하단이 0, 앞이 x

    template <typename T>
    class GameMap
    {
        const short SECTOR_SIZE = 100;
        const short MAP_SIZE = 0;
        const short MAX_SECTOR_INDEX_X = MAP_SIZE / SECTOR_SIZE;
        const short MAX_SECTOR_INDEX_Y = MAP_SIZE / SECTOR_SIZE;

        const short GRID_SIZE = 100;

    private:
        decltype(auto) GetValidSectorView()
        {
            return std::views::filter([this](Sector sector)
                   {
                       return IsValidSector(sector);
                   })
                   | std::views::transform([this](Sector sector)
                   {
                       return _map[sector.x][sector.y];
                   });
        }

        decltype(auto) FlatRange(Sector begin, Sector end)
        {
            return std::views::iota(begin.x, end.x + 1)
                   | std::views::transform([=](auto x)
                   {
                       return std::views::iota(begin.y, end.y + 1)
                              | std::views::transform([=](auto y)
                              {
                                  return Sector{x, y};
                              });
                   })
                   | std::views::join;
        }

        decltype(auto) SectorsView(const FVector& point1, const FVector& point2)
        {
            const auto p1Sector = GetSector(point1);
            const auto p2Sector = GetSector(point2);

            return FlatRange(p1Sector, p2Sector);
        }

        decltype(auto) SectorsView(const Sector& target, std::span<const Sector> offsets) const
        {
            // Convert offset to sectors
            return std::views::all(offsets)
                   | std::views::transform([&](const Sector offset)
                   {
                       const short x = target.x + offset.x;
                       const short y = target.y + offset.y;
                       return Sector{x, y};
                   });
        }

    public:
        FVector GetRandomLocation() const
        {
            return {RandomUtil::Rand(0, MAP_SIZE - 100) + 50.0f, RandomUtil::Rand(0, MAP_SIZE - 100) + 50.0f};
        }

        using container = flat_unordered_set<T>;

        GameMap(short mapSize, short sectorSize)
            : SECTOR_SIZE(sectorSize)
            , MAP_SIZE(mapSize)
            , MAX_SECTOR_INDEX_X(MAP_SIZE / SECTOR_SIZE - 1)
            , MAX_SECTOR_INDEX_Y(MAP_SIZE / SECTOR_SIZE - 1)
        {
            ASSERT_CRASH(SECTOR_SIZE > 0, "Invalid Sector Size");
            _map.resize(MAP_SIZE / SECTOR_SIZE);
            for (auto& i : _map)
            {
                i.resize(MAP_SIZE / SECTOR_SIZE);
            }
        }

        ~GameMap() = default;

        void PrintPlayerInfo()
        {
            const auto tmp = GetPlayerInfo();

            for (auto& col : tmp)
            {
                for (const auto& sector : col)
                {
                    std::cout << sector << ' ';
                }
                std::cout << '\n';
            }
            std::cout << '\n';
        }

        vector<vector<int>> GetPlayerInfo()
        {
            vector ret(MAX_SECTOR_INDEX_Y+1, vector<int>(MAX_SECTOR_INDEX_X+1));

            for (int i = 0; i <= MAX_SECTOR_INDEX_X; i++)
            {
                for (int j = 0; j <= MAX_SECTOR_INDEX_Y; j++)
                {
                    ret[MAX_SECTOR_INDEX_X - i][j] = _map[i][j].size();
                }
            }
            return ret;
        }

        short Size() const
        {
            return MAP_SIZE;
        }

        int Players()
        {
            int count = 0;
            for (auto& col : _map)
            {
                for (auto& row : col)
                {
                    count += row.size();
                }
            }
            return count;
        }


        [[nodiscard]] Sector GetSector(FVector location) const
        {
            //입력이 비정상이면 비정상적인 값 줌.
            //여기가 모든 입력을 걸러준다는 가정. 
            if (location.X < 0 || MAP_SIZE <= location.X || location.Y < 0 || MAP_SIZE <= location.Y)
            {
                return {-1, -1};
            }

            return {static_cast<short>(location.X / SECTOR_SIZE), static_cast<short>(location.Y / SECTOR_SIZE)};
        }

        decltype(auto) GetSectorsFromRange(const Range& attackRange)
        {
            auto objectMapPtr = reinterpret_cast<GameMap<GameObject>*>(this);
            auto view = std::views::all(attackRange.getSectors(*objectMapPtr)) | GetValidSectorView();
            return view;
        }

        decltype(auto) GetSectorsFromPoint(const FVector& p1, const FVector& p2)
        {
            auto sectors = SectorsView(p1, p2);
            return sectors | GetValidSectorView();
        }

        decltype(auto) GetSectorsFromOffset(const Sector& target, std::span<const Sector> offsets)
        {
            auto sectors = SectorsView(target, offsets);
            return sectors | GetValidSectorView();
        }

        void Insert(const T& target, FVector location)
        {
            const auto targetSector = GetSector(location);

            ASSERT_CRASH(0 <= targetSector.x && targetSector.x <=MAX_SECTOR_INDEX_X, "Invalid xLocation");
            ASSERT_CRASH(0 <= targetSector.y && targetSector.x <=MAX_SECTOR_INDEX_Y, "Invalid yLocation");

            _map[targetSector.x][targetSector.y].insert(target);
        }

        void Delete(const T& target, FVector location)
        {
            const auto targetSector = GetSector(location);

            ASSERT_CRASH(0 <= targetSector.x && targetSector.x <=MAX_SECTOR_INDEX_X, "Invalid xLocation");
            ASSERT_CRASH(0 <= targetSector.y && targetSector.x <=MAX_SECTOR_INDEX_Y, "Invalid yLocation");

            _map[targetSector.x][targetSector.y].erase(target);
        }

        void ClamToMap(FVector& loc)
        {
            loc = Clamp(loc, 0, MAP_SIZE-1);
        }

        bool IsValidSector(Sector sector) const
        {
            if (sector.x < 0 || sector.y < 0)
            {
                return false;
            }

            if (MAX_SECTOR_INDEX_X < sector.x || MAX_SECTOR_INDEX_Y < sector.y)
            {
                return false;
            }

            return true;
        }

    private:
        GameMap(const GameMap& other) = delete;
        GameMap(GameMap&& other) noexcept = delete;
        GameMap& operator=(const GameMap& other) = delete;
        GameMap& operator=(GameMap&& other) noexcept = delete;

    private:
        std::vector<std::vector<container>> _map;
    };
}
