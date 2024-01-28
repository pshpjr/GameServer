#include "stdafx.h"
#include "GameMap.h"
#include "Player.h"

struct point 
{
	int8 x;
	int8 y;
};



/*
* 789
* 456
* 123
* 철권식 표기를 위해 enum에선 반대로 정의함.
*/
enum Direction : int8
{
	LD, DD, RD,
	LL, Center, RR,
	LU, UU, RU
};

constexpr point dxdyTable[9] =
{ 
	{ -1, -1},{ -1, 0},{ -1, 1},
	{ 0 , -1},{ 0 , 0},{ 0 , 1},
	{ 1 , -1},{ 1 , 0},{ 1 , 1}
};

consteval point Offset(const Direction dir) 
{
	return dxdyTable[dir];
}

void psh::GameMap::InvokeNearbyPlayer(const Sector targetSector, std::function<void(void)> toInvoke)
{
	for (auto offset : dxdyTable)
	{
		if ( targetSector - offset)
	}



}

void psh::GameMap::AddPlayer(Player& newPlayer)
{
	auto targetSector = CalculatePlayerSector(newPlayer);


}

psh::Sector psh::GameMap::CalculatePlayerSector(const Player& player)
{
	short SectorX = player.location.X / SECTOR_SIZE;
	short SectorY = player.location.Y / SECTOR_SIZE;

	ASSERT_CRASH(SectorX <= MAX_SECTOR_INDEX_X, "invalid player LocationX");
	ASSERT_CRASH(SectorY <= MAX_SECTOR_INDEX_Y, "invalid player LocationY");

	return Sector(SectorX, SectorY);
}

bool psh::GameMap::isVaildSector(const Sector sector)
{
	if (sector.x < 0 || sector.y < 0)
		return false;

	if (MAX_SECTOR_INDEX_X < sector.x || MAX_SECTOR_INDEX_Y < sector.y)
		return false;

	return true;
}
