#pragma once

namespace psh 
{
	//sector�� ��ȿ �˻�� Map����
	struct Sector
	{
		bool operator== ( const Sector& other) const
		{
			return x == other.x
				&& y == other.y;
		}

		bool operator!= ( const Sector& other) const
		{
			return !(*this == other);
		}

		Sector operator- ( const Sector& other) const
		{
			return {static_cast<short>(x - other.x) , static_cast<short>(y - other.y)};
		}

		Sector operator+ ( const Sector& other) const
		{
			return {static_cast<short>(x + other.x) , static_cast<short>(y + other.y)};
		}
		
		short x;
		short y;

	};
}
