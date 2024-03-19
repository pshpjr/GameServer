#include "stdafx.h"
#include "Player.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "ObjectManager.h"

#include "../GameMap.h"
#include "../Data/AttackData.h"
#include "../Data/TableData.h"


namespace psh
{
    constexpr float PLAYER_MOVE_SPEED = 400;

    Player::Player(ObjectID id
            , ObjectManager& owner
            , GroupCommon& group
                            , FVector location
                            , DBData& data)
        : ChatCharacter(id, owner,group,location,
            PLAYER_MOVE_SPEED, eCharacterGroup::Player, data.CharacterType())
        , _data(data)
    {
        _attacks = playerAttack[data.CharacterType()];
    }

    void Player::GetCoin(char value)
    {
        auto getCoin = SendBuffer::Alloc();
        MakeGame_ResGetCoin(getCoin, ObjectId(), 1);
        _group.SendPacket(SessionId(), getCoin);
        
        _data.AddCoint(value);
    }

    void Player::MakeCreatePacket(SendBuffer& buffer, bool spawn) const
    {
        MakeGame_ResCreateActor(buffer, ObjectId(), ObjectGroup(), Type(), Location(), Direction(), Destination(), isMove(), spawn,_data.Nick());

    }

    void Player::OnDestroy() const
    {
        auto die = SendBuffer::Alloc();
        MakeGame_ResDestroyActor(die, ObjectId(), true,1);
        _group.SendPacket(SessionId(), die);
    }
    

}
