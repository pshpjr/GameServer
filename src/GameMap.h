#pragma once

#include <span>
#include <vector>

#include "FVector.h"
#include "Sector.h"



namespace psh
{
    struct FVector;

    //2차원 고정 크기 그리드 방식 게임 맵.
    //언리얼의 좌표계는 y가 오른쪽. 좌하단이 0, 앞이 x
    //sectorView는 게임 맵의 여러 섹터들을 추상화 해서 하나의 view로 바라볼 수 있게 함(range::view와 유사)
    template <typename keyTy, typename valTy>
    class GameMap
    {
        const short SECTOR_SIZE = 100;
        const short MAP_SIZE = 0;
        const short MAX_SECTOR_INDEX_X = MAP_SIZE / SECTOR_SIZE;
        const short MAX_SECTOR_INDEX_Y = MAP_SIZE / SECTOR_SIZE;

        const short GRID_SIZE = 100;
        using container = std::unordered_map<keyTy, valTy>;

    public:

        class SectorViewIterator
        {
            using iterator_category = std::forward_iterator_tag;
            using value_type = valTy;
            using outerIterator = typename std::vector<std::reference_wrapper<container>>::iterator;
            using innerIterator = typename container::iterator;
            using reference = value_type&;
            using pointer = value_type*;

        public:
            SectorViewIterator(outerIterator begin, outerIterator end)
                : _cur{begin}
                , _end{end}
            {
                if (_cur != _end)
                {
                    _it = _cur->get().begin();
                    moveNext();
                }
            }

            SectorViewIterator& operator++()
            {
                ++_it;
                moveNext();
                return *this;
            }

            SectorViewIterator operator++(int)
            {
                SectorViewIterator temp = *this;
                ++(*this);
                return temp;
            }

            reference operator*()
            {
                return (*_it).second;
            }

            pointer operator->()
            {
                return &((*_it).second);
            }

            friend bool operator==(const SectorViewIterator& lhs, const SectorViewIterator& rhs)
            {
                // 두 반복자가 모두 끝에 도달한 경우
                if (lhs._cur == lhs._end && rhs._cur == rhs._end)
                {
                    return true;
                }

                // 하나의 반복자가 끝에 도달한 경우
                if (lhs._cur == lhs._end || rhs._cur == rhs._end)
                {
                    return false;
                }

                // 현재 섹터와 객체 위치가 동일한 경우
                return lhs._cur == rhs._cur && lhs._it == rhs._it;
            }

            friend bool operator!=(const SectorViewIterator& lhs, const SectorViewIterator& rhs)
            {
                return !(lhs == rhs);
            }

        private:
            void moveNext()
            {
                while (_cur != _end && _it == _cur->get().end())
                {
                    ++_cur;
                    if (_cur != _end)
                    {
                        _it = _cur->get().begin();
                    }
                }
            }

        private:
            outerIterator _cur;
            outerIterator _end;
            innerIterator _it;
        };

        //만든 가장 큰 원인은 cpp20의 view가 디버깅 하기 어렵다는 점 때문.
        //지연 초기화 때문에 왜 이렇게 되는지 알 수 가 없음.
        //생성 시점에 섹터가 고정된다는 점을 빼면 동일함.
        class SectorView
        {
        public:
            using sectorContainer = std::vector<std::reference_wrapper<container>>;

            using iterator = SectorViewIterator;

            explicit SectorView() = default;

            explicit SectorView(sectorContainer& sectors)
                : _sectors(sectors) {}

            //중복된 섹터를 넣을 경우 중복해서 탐색함.
            explicit SectorView(const std::vector<SectorView>& views)
            {
                for (auto& view : views)
                {
                    _sectors.insert(_sectors.end(), view._sectors.begin(), view._sectors.end());
                }
            }

            iterator begin()
            {
                return iterator(_sectors.begin(), _sectors.end());
            }

            iterator end()
            {
                return iterator(_sectors.end(), _sectors.end());
            }

        private:
            sectorContainer _sectors{};
        };

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

        /**
         * nxn 2차원 배열의 n 크기를 리턴함.
         * @return
         */
        [[nodiscard]] short GetMapSize() const
        {
            return MAP_SIZE;
        }

        /**
         * 저장된 객체의 수를 리턴함.
         * @return 저장된 객체 개수
         */
        [[nodiscard]] int GetObjectCount() const
        {
            return _objects;
        }

        //이 위치는 어떤 섹터에 있나요?
        [[nodiscard]] Sector GetSectorAtLocation(FVector location) const
        {
            //입력이 비정상이면 비정상적인 값 줌.
            //여기가 모든 입력을 걸러준다는 가정.
            if (location.X < 0 || MAP_SIZE <= location.X || location.Y < 0 || MAP_SIZE <= location.Y)
            {
                //나중에 invalid 왜 생기나 확인하기.
                ClampLocationToMap(location);
            }

            return {static_cast<short>(location.X / SECTOR_SIZE), static_cast<short>(location.Y / SECTOR_SIZE)};
        }

        //맵 벗어나지 못하게
        void ClampLocationToMap(FVector& loc) const
        {
            loc = Clamp(loc, 0, static_cast<float>(MAP_SIZE - 1));
        }

        /**
         *
         * @param target 삽입할 객체
         * @param location 위치
         */
        void Insert(const keyTy key, const valTy& val, const FVector location)
        {
            ++_objects;
            const auto [x,y] = GetSectorAtLocation(location);

            ASSERT_CRASH(0 <= x && x <=MAX_SECTOR_INDEX_X, "Invalid xLocation");
            ASSERT_CRASH(0 <= y && y <=MAX_SECTOR_INDEX_Y, "Invalid yLocation");

            _map[x][y].insert({key, val});
        }

        //객체 삭제
        void Delete(const keyTy key, const FVector location)
        {
            --_objects;
            const auto [x,y] = GetSectorAtLocation(location);

            ASSERT_CRASH(0 <= x && x <=MAX_SECTOR_INDEX_X, "Invalid xLocation");
            ASSERT_CRASH(0 <= y && y <=MAX_SECTOR_INDEX_Y, "Invalid yLocation");

            _map[x][y].erase(key);
        }

        [[nodiscard]] bool IsSectorValid(const Sector sector) const
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

        //begin, end를 포함하는 범위의 섹터를 사각형 형태로 선택함.
        static std::vector<Sector> GetSectorsInRange(const Sector& begin, const Sector& end)
        {
            std::vector<Sector> sectors;
            for (short x = begin.x; x <= end.x; ++x)
            {
                for (short y = begin.y; y <= end.y; ++y)
                {
                    sectors.emplace_back(Sector{x, y});
                }
            }
            return sectors;
        }

        //두 점을 포함하는 섹터를 사각형 형태로 선택함.
        SectorView CreateSectorViewBetweenPoints(const FVector& p1, const FVector& p2)
        {
            auto sectors = GetSectorsBetweenPoints(p1, p2);
            return CreateSectorView(sectors);
        }

        //타겟 섹터와 offset 값에 있는 섹터들을 선택함. 0,0도 추가해야 타겟 섹터도 선택됨. 중복 생길 수 있음.
        SectorView CreateSectorViewWithOffsets(const Sector& target, const std::span<const Sector> offsets)
        {
            auto sectors = GetAdjacentSectors(target, offsets);
            return CreateSectorView(sectors);
        }

        //타겟 점을 기준으로 offset 값에 있는 섹터들을 선택함. 0,0도 추가해야 타겟 섹터도 선택됨. 중복 생길 수 있음.
        SectorView CreateSectorViewWithOffsets(const FVector& location, const std::span<const Sector> offsets)
        {
            const Sector target = GetSectorAtLocation(location);
            return CreateSectorViewWithOffsets(target, offsets);
        }

        //해당 점들을 포함하는 뷰를 생성. 중복 포함하지 않음.
        SectorView CreateSectorViewFromLocations(const std::list<FVector>& locations)
        {
            std::set<Sector> unique_sectors;
            for (const auto& location : locations)
            {
                unique_sectors.insert(GetSectorAtLocation(location));
            }
            std::vector sectors(unique_sectors.begin(), unique_sectors.end());
            return CreateSectorView(sectors);
        }

    private:

        SectorView CreateSectorView(const std::vector<Sector>& sectors)
        {
            typename SectorView::sectorContainer sectorContainers;
            for (const auto& sector : sectors)
            {
                if (IsSectorValid(sector))
                {
                    sectorContainers.push_back(_map[sector.x][sector.y]);
                }
            }
            return SectorView(sectorContainers);
        }


        [[nodiscard]] std::vector<Sector> GetSectorsBetweenPoints(const FVector& point1, const FVector& point2) const
        {
            const auto p1Sector = GetSectorAtLocation(point1);
            const auto p2Sector = GetSectorAtLocation(point2);
            return GetSectorsInRange(p1Sector, p2Sector);
        }


        static std::vector<Sector> GetAdjacentSectors(const Sector& target, const std::span<const Sector> offsets)
        {
            std::vector<Sector> sectors;
            for (const auto& offset : offsets)
            {
                const auto x = static_cast<short>(target.x + offset.x);
                const auto y = static_cast<short>(target.y + offset.y);
                sectors.emplace_back(Sector{x, y});
            }
            return sectors;
        }

        std::vector<std::vector<container>> _map;
        int _objects = 0;
    };
}
