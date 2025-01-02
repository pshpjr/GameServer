#include "Range.h"

#include <format>
#include <fstream>

#include "Macro.h"
#include "PacketGenerated.h"

namespace psh
{
    SquareRange::SquareRange(const FVector p1) : SquareRange{{0, 0}, p1}
    {
    }

    SquareRange::SquareRange(FVector p1, FVector p2)
        : Range()
    {
        //p1 < p2;
        //p1은 좌하단, p2는 우상단
        if (p1 > p2)
        {
            std::swap(p1, p2);
        }
        _p1 = p1;
        _p2 = p2;

        //p1부터 반시계방향
        _points = {_p1, {_p1.X, _p2.Y}, _p2, {_p2.X, _p1.Y}};
        _midPoints = {
            {_p1.X, (_p1.Y + _p2.Y) / 2}
          , {(_p1.X + _p2.X) / 2, _p2.Y}
          , {_p2.X, (_p1.Y + _p2.Y) / 2}
          , {(_p1.X + _p2.X) / 2, _p1.Y}
        };
    }

    void SquareRange::Rotate(FVector rotation, FVector origin) noexcept
    {
        // std::ranges::transform(_points, _points.begin(), [rotation, origin](const FVector& point)
        // {
        //     return psh::Rotate(point, rotation, origin);
        // });
        _rotations.push_back({rotation, origin});
        std::ranges::transform(_midPoints, _midPoints.begin(), [rotation,origin](const FVector point) {
            return psh::Rotate(point, rotation, origin);
        });
    }


    //성능 최적화를 위해 회전 시 점을 옮기는 게 아니라 회전 정보를 저장함.
    bool SquareRange::Contains(const FVector p) const noexcept
    {
        auto point = p;
        for (auto &[rotate,origin]: _rotations)
        {
            point = ReverseRotate(point, rotate, origin);
        }

        return (_p1.X < point.X && point.X <= _p2.X)
               && (_p1.Y < point.Y && point.Y <= _p2.Y);
    }

    //TODO: 이진 ccw 최적화. 점 4개면 그냥 시계, 반시계 돌아서 비교하면 끝.
    bool SquareRange::oldContaion(FVector p) const noexcept
    {
        ASSERT_CRASH(_points.size() > 2, L"Range Need Points greater then 2");

        // Determine initial direction.
        const bool initialDirection = CCW(_points[0], _points[1], p) >= 0;

        // Use binary search to find the last index maintaining the initial direction.
        auto low = 1;

        auto high = static_cast<int>(_points.size()) - 1;

        while (high - low > 1)
        {
            if (const int mid = (low + high) / 2;
                (CCW(_points[0], _points[mid], p) >= 0) == initialDirection)
            {
                low = mid;
            }
            else
            {
                high = mid;
            }
        }

        const auto result1 = (CCW(_points[0], _points[low], p) >= 0) == initialDirection;
        const auto result2 = (CCW(_points[low], _points[high], p) >= 0) == initialDirection;
        const auto result3 = (CCW(_points[high], _points[0], p) >= 0) == initialDirection;

        // Check if point is inside by confirming consistent direction in both sides.
        return result1 && result2 && result3;
    }


    std::list<FVector> SquareRange::GetCoordinates() const
    {
        return _midPoints;
    }

    SquareRange &SquareRange::operator+=(const FVector vector)
    {
        _p1 += vector;
        _p2 += vector;

        for (auto &p: _points)
        {
            p += vector;
        }
        return *this;
    }

    void SquareRange::DrawRangeIntoBuffer(SendBuffer &buffer) const
    {
        auto drawPoint = _points;

        for (auto [rotation,origin]: _rotations)
        {
            std::ranges::transform(drawPoint, drawPoint.begin(), [rotation,origin](const FVector point) {
                return psh::Rotate(point, rotation, origin);
            });
        }


        for (const auto &point: drawPoint)
        {
            MakeGame_ResDraw(buffer, point);
        }
    }

    void SquareRange::printInfo(std::ostream &os) const
    {
        os << '[' << _points[0] << ',' << _points[1] << ',' << _points[2] << ',' << _points[3] << ']';
    }

    CircleRange::CircleRange(const FVector &point, const float radius)
        : Range()
      , _point(point)
      , _radius(radius)
      , _keyPoints({
                       {point.X, point.Y + radius}
                     , {point.X + radius, point.Y + radius}
                     , {point.X, point.Y - radius}
                     , {point.X - radius, point.Y - radius}
                   })
    {
    }


    bool CircleRange::Contains(const FVector p) const noexcept
    {
        return Distance(_point, p) <= _radius;
    }

    std::list<FVector> CircleRange::GetCoordinates() const
    {
        return _keyPoints;
    }

    Range &CircleRange::operator+=(const FVector vector)
    {
        _point += vector;
        std::ranges::transform(_keyPoints, _keyPoints.begin(), [vector](const FVector key) {
            return key + vector;
        });
        return *this;
    }
}
