#include "Range.h"

#include <format>
#include <fstream>

#include "Macro.h"
#include "PacketGenerated.h"

namespace psh
{
	SquareRange::SquareRange(FVector p1) : SquareRange{{0, 0}, p1}
	{
	}

	SquareRange::SquareRange(FVector p1, FVector p2)
	{
		if (p1 > p2)
		{
			std::swap(p1, p2);
		}
		_p1 = p1;
		_p2 = p2;
		_xLen = p2.X - p1.X;
		_yLen = p2.Y - p2.X;
		//p1부터 반시계방향
		_points = {_p1, {_p1.X, _p2.Y}, _p2, {_p2.X, _p1.Y}};
	}

	void SquareRange::Rotate(FVector rotation, FVector origin) noexcept
	{
		std::ranges::transform(_points, _points.begin(), [rotation, origin](const FVector& point)
		{
			return psh::Rotate(point, rotation, origin);
		});
	}

	bool SquareRange::Contains(FVector p) const noexcept
	{
		FVector Center = (_points[0] + _points[2]) / 2;
		FVector CenterDir = Center - p;

		// 첫 번째 축에 대한 내적 및 비교
		auto axis = _points[1] - _points[0];
		auto normal = {axis.Y, -axis.X};
		float CenterProjDist = abs(Dot(CenterDir, axis));
		if (CenterProjDist > _xLen)
			return false;

		// 두 번째 축에 대한 내적 및 비교
		axis = _points[2] - _points[3];
		CenterProjDist = abs(Dot(CenterDir, axis));
		if (CenterProjDist > _yLen)
			return false;

		return true;
	}


	std::list<FVector> SquareRange::GetCoordinates() const
	{
		return {
			Mid(_points[0], _points[1]),
			Mid(_points[1], _points[2]),
			Mid(_points[2], _points[3]),
			Mid(_points[3], _points[4])
		};
	}

	SquareRange& SquareRange::operator+=(FVector vector)
	{
		_p1 += vector;
		_p2 += vector;

		for (auto& p : _points)
		{
			p += vector;
		}
		return *this;
	}

	void SquareRange::DrawRangeIntoBuffer(SendBuffer& buffer) const
	{
		auto drawPoint = _points;

		for (const auto& point : drawPoint)
		{
			MakeGame_ResDraw(buffer, point);
		}
	}

	void SquareRange::printInfo(std::ostream& os) const
	{
		os << '[' << _points[0] << ',' << _points[1] << ',' << _points[2] << ',' << _points[3] << ']';
	}

	CircleRange::CircleRange(const FVector& point, const float radius)
		: Range()
		  , _point(point)
		  , _radius(radius)
		  , _keyPoints({
			  {point.X, point.Y + radius}, {point.X + radius, point.Y + radius}, {point.X, point.Y - radius},
			  {point.X - radius, point.Y - radius}
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

	Range& CircleRange::operator+=(const FVector vector)
	{
		_point += vector;
		std::ranges::transform(_keyPoints, _keyPoints.begin(), [vector](const FVector key)
		{
			return key + vector;
		});
		return *this;
	}
}
