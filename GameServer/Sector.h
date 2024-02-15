#pragma once

namespace psh 
{
	//sector의 유효 검사는 Map에서
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
