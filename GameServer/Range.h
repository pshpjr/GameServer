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
        return CCW(_points[0], _points[1], p) >= 0
            && CCW(_points[1], _points[2], p) >= 0
            && CCW(_points[2], _points[3], p) >= 0
            && CCW(_points[3], _points[0], p) >= 0;
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
