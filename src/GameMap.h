#pragma once

#include <iostream>
#include <vector>

#include <range/v3/all.hpp>
#include "FVector.h"
#include "Sector.h"


namespace psh
{
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

        decltype(auto) GetDataViewFromSectors()
        {
            return ranges::views::filter([this](const Sector sector)
                {
                    return IsValidSector(sector);
                })
                | ranges::views::transform([this](Sector sector)
                {
                    return ranges::views::all(_map[sector.x][sector.y]);
                })
                | ranges::views::join;
        }


        static decltype(auto) GetFlatSectorsView(const Sector begin, const Sector end)
        {
            return ranges::views::iota(begin.x, end.x + 1)
                | ranges::views::transform([=](auto x)
                {
                    return ranges::views::iota(begin.y, end.y + 1)
                        | ranges::views::transform([=](auto y)
                        {
                            return Sector{x, y};
                        });
                })
                | ranges::views::join;
        }

        decltype(auto) SectorsViewByPoint(const FVector& point1, const FVector& point2) const
        {
            const auto p1Sector = GetSector(point1);
            const auto p2Sector = GetSector(point2);

            return GetFlatSectorsView(p1Sector, p2Sector);
        }

        static decltype(auto) SectorsViewBySector(const Sector& target, std::span<const Sector> offsets)
        {
            // target의 수명이 어떨지 모르므로 람다에서 값으로 복사
            return ranges::views::all(offsets)
                | ranges::views::transform([target](const Sector offset)
                {
                    const auto x = static_cast<short>(target.x + offset.x);
                    const auto y = static_cast<short>(target.y + offset.y);
                    return Sector{x, y};
                });
        }

    public:
        using container = std::unordered_set<T>;

        GameMap(const GameMap& other) = delete;
        GameMap(GameMap&& other) noexcept = delete;
        GameMap& operator=(const GameMap& other) = delete;
        GameMap& operator=(GameMap&& other) noexcept = delete;

        //using container = std::flat_unordered_set<T>;
        //        using container = vector<T>;
        GameMap(const short mapSize, const short sectorSize)
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

        [[nodiscard]] short Size() const
        {
            return MAP_SIZE;
        }

        [[nodiscard]] int Objects() const
        {
            return _objects;
        }

        [[nodiscard]] Sector GetSector(const FVector location) const
        {
            //입력이 비정상이면 비정상적인 값 줌.
            //여기가 모든 입력을 걸러준다는 가정. 
            if (location.X < 0 || MAP_SIZE <= location.X || location.Y < 0 || MAP_SIZE <= location.Y)
            {
                std::cout << "InvalidSector" << '\n';
                return {-1, -1};
            }

            return {static_cast<short>(location.X / SECTOR_SIZE), static_cast<short>(location.Y / SECTOR_SIZE)};
        }

        void ClampToMap(FVector& loc) const
        {
            loc = Clamp(loc, 0, static_cast<float>(MAP_SIZE - 1));
        }

        decltype(auto) GetSectorsFromPoint(const FVector& p1, const FVector& p2)
        {
            auto sectors = SectorsViewByPoint(p1, p2);
            return sectors | GetDataViewFromSectors();
        }

        decltype(auto) GetSectorsFromOffset(const Sector& target, const std::span<const Sector> offsets)
        {
            auto sectors = SectorsViewBySector(target, offsets);
            return sectors | GetDataViewFromSectors();
        }

        //offset을 span으로 받으므로 offsets의 수명은 이 view가 실행될 때 까지 살아있어야 함. 
        decltype(auto) GetSectorsFromOffset(const FVector& location, const std::span<const Sector> offsets)
        {
            const Sector target = GetSector(location);
            return GetSectorsFromOffset(target, offsets);
        }

        decltype(auto) GetSectorsByList(const std::list<FVector> &locations)
        {
            std::list<Sector> unique_list = locations
                | ranges::views::transform([this](auto &location) {
                    return GetSector(location);
                })| ranges::to<std::list>();

            unique_list.sort();
            unique_list.unique();

            return unique_list
                | GetDataViewFromSectors();
        }



        void Insert(const T& target, const FVector location)
        {
            ++_objects;
            const auto [x,y] = GetSector(location);

            ASSERT_CRASH(0 <= x && x <=MAX_SECTOR_INDEX_X, "Invalid xLocation");
            ASSERT_CRASH(0 <= y && y <=MAX_SECTOR_INDEX_Y, "Invalid yLocation");

            _map[x][y].insert(target);
        }

        void Delete(const T& target, const FVector location)
        {
            --_objects;
            const auto [x,y] = GetSector(location);

            ASSERT_CRASH(0 <= x && x <=MAX_SECTOR_INDEX_X, "Invalid xLocation");
            ASSERT_CRASH(0 <= y && y <=MAX_SECTOR_INDEX_Y, "Invalid yLocation");

            _map[x][y].erase(target);
        }



        [[nodiscard]] bool IsValidSector(const Sector sector) const
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

        std::vector<std::vector<container>> _map;
        int _objects = 0;
    };
}
