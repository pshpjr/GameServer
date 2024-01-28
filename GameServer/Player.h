#pragma once
#include "ContentTypes.h"



//player�� � ���� �ùٸ��� �𸣴ϱ� �ܺο��� �ùٸ� ������ �־���� �ϴ°�?
//player�� map������ �����ϴ°�?

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

