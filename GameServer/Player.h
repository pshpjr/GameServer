#pragma once
#include "ContentTypes.h"



//player는 어떤 값이 올바른지 모르니까 외부에서 올바른 값으로 넣어줘야 하는가?
//player는 map에서만 생성하는가?

namespace psh 
{
	class Player
	{
		friend Sector GameMap::CalculatePlayerSector(const Player& player);
	public:
		void Attack();
		void Move();
		void Die();

		

	private:


		psh::FVector location;
		psh::FVector direction;
		psh::FVector vector;
	};


}

