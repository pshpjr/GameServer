#include "Range.h"
#include "../GameMap.h"
#include "../Sector.h"
namespace psh
{
	std::vector<psh::Sector> SquareRange::getSectors(const GameMap& map) const
	{
		
		std::unordered_set<psh::Sector> unique_sectors;
		for (const auto& point : _points) 
		{
			unique_sectors.insert(map.GetSector(point));
		}

		return std::vector<psh::Sector>(unique_sectors.begin(), unique_sectors.end());
	
	}
}
