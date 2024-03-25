#pragma once
#include "ContentTypes.h"
#include "FVector.h"

namespace psh
{
    struct Sector;

    template <typename T>
    class GameMap;
    class GameObject;

    struct Range
    {
        virtual ~Range() = default;
        virtual bool Contains(FVector p) const noexcept = 0;
        virtual std::vector<Sector> getSectors(GameMap<GameObject>& map) const = 0;
    };


    struct SquareRange : public Range
    {
        SquareRange(FVector p1, FVector p2)
        {
            //p1 < p2;
            if (p1 > p2)
            {
                swap(p1, p2);
            }
            _p1 = p1;
            _p2 = p2;

            _points = {_p1, {_p1.X, _p2.Y}, _p2, {_p2.X, _p1.Y}};
        }

        void Rotate(FVector rotation, FVector origin = {0, 0}) noexcept
        {
            ranges::for_each(_points, [this,rotation,origin](FVector& point)
            {
                point = psh::Rotate(point, rotation, origin);
            });
        }

        ~SquareRange() override = default;
        std::vector<Sector> getSectors(GameMap<GameObject>& map) const override;


        bool Contains(FVector p) const noexcept override;

        FVector _p1;
        FVector _p2;
        vector<FVector> _points;

        SquareRange& operator+=(const FVector& vector);
    };

    inline bool SquareRange::Contains(const FVector p) const noexcept
    {
        ASSERT_CRASH(_points.size() > 2, L"Range Need Points greater then 2");

        // Determine initial direction.
        bool initialDirection = CCW(_points[0], _points[1], p) >= 0;

        // Use binary search to find the last index maintaining the initial direction.
        int low = 1;

        int high = static_cast<int>(_points.size()) - 1;

        while (high - low > 1)
        {
            int mid = (low + high) / 2;
            if ((CCW(_points[0], _points[mid], p) >= 0) == initialDirection)
            {
                low = mid;
            }
            else
            {
                high = mid;
            }
        }

        auto result1 = (CCW(_points[0], _points[low], p) >= 0) == initialDirection;
        auto result2 = (CCW(_points[low], _points[high], p) >= 0) == initialDirection;
        auto result3 = (CCW(_points[high], _points[0], p) >= 0) == initialDirection;

        // Check if point is inside by confirming consistent direction in both sides.
        return result1 && result2 && result3;
    }

    inline SquareRange& SquareRange::operator+=(const FVector& vector)
    {
        _p1 += vector;
        _p2 += vector;

        for (auto& p : _points)
        {
            p += vector;
        }
        return *this;
    }

    struct CircleRange : public Range
    {
        FVector point;
        float radius;

        CircleRange(const FVector& point, float radius);

        std::vector<Sector> getSectors(GameMap<GameObject>& map) const override;

        bool Contains(FVector p) const noexcept override;
    };
}
