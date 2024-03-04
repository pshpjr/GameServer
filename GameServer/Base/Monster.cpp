#include "Monster.h"
#include "../GameMap.h"
namespace psh 
{
    constexpr int PLAYER_MOVE_SPEED = 400;
	
    Monster::Monster(ObjectID clientId, const psh::FVector& location, const psh::FVector& direction, char type)
    : ChatCharacter(clientId,location,direction,PLAYER_MOVE_SPEED,eCharacterGroup::Monster,type)
    {
        _attacks =
        {
            {{200,200},10},
            {{200,200},20},			
            {{400,200},40},	
            {{500,200},10},	
        };  
    }

 

    void Monster::OnUpdate(float delta)
    {
        if(RandomUtil::Rand(0,100) > 99)
        {
            Attack(0);
        }
    }

    void Monster::Die()
    {
        ChatCharacter::Die();
    }

    void Monster::OnMove()
    {
        ChatCharacter::OnMove();
    }

    void Monster::MoveStart(FVector destination)
    {
        ChatCharacter::MoveStart(destination);
    }
}
