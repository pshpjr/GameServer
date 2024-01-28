#pragma once
#include "Sector.h"
#include <functional>

class Player;

namespace psh 
{
	class GameMap
	{
		static constexpr int SECTOR_SIZE = 100;
		static constexpr int MAP_SIZE = 6400;

		static constexpr int MAX_SECTOR_INDEX_X = MAP_SIZE/SECTOR_SIZE - 1;
		static constexpr int MAX_SECTOR_INDEX_Y = MAP_SIZE/SECTOR_SIZE - 1;

	public:
		GameMap() :_map(MAP_SIZE / SECTOR_SIZE, std::vector<HashSet<Player*>>(MAP_SIZE / SECTOR_SIZE))
		{

		}

		void InvokeNearbyPlayer(const Sector targetSector, std::function<void(void)> toInvoke);

		void AddPlayer(Player& player);


		//적절한 위치인가?
		static Sector CalculatePlayerSector(const Player& player);
		static bool isVaildSector(const Sector sector);
	private:


		//map은 객체의 주인이 아님. 
		vector < vector < HashSet< Player*>>> _map;
	};

}

