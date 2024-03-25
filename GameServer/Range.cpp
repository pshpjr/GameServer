#include "Range.h"
#include "GameMap.h"
#include "Sector.h"

namespace psh
{
    std::vector<Sector> SquareRange::getSectors(GameMap<GameObject>& map) const
    {
        std::unordered_set<Sector> unique_sectors;
        for (const auto& point : _points)
        {
            unique_sectors.insert(map.GetSector(point));
        }

        return std::vector<Sector>(unique_sectors.begin(), unique_sectors.end());
    }

    CircleRange::CircleRange(const FVector& point, float radius): point(point)
                                                                , radius(radius)
    {
    }

    std::vector<Sector> CircleRange::getSectors(GameMap<GameObject>& map) const
    {
        array<FVector, 4> points = {
            {
                {point.X, point.Y + radius}
                , {point.X + radius, point.Y + radius}
                , {point.X, point.Y - radius}
                , {point.X - radius, point.Y - radius}}};

        std::unordered_set<Sector> unique_sectors;
        for (const auto& point : points)
        {
            unique_sectors.insert(map.GetSector(point));
        }

        return std::vector<Sector>(unique_sectors.begin(), unique_sectors.end());
    }

    bool CircleRange::Contains(const FVector p) const noexcept
    {
        return Distance(point, p) <= radius;
    }
}
