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
    constexpr float PLAYER_MOVE_SPEED = 200;

    Player::Player(ObjectID id
            , ObjectManager& owner
            , GroupCommon& group
                            , FVector location
                            , shared_ptr<DBData> data
                            , DBThreadWrapper* dbThread)
        : ChatCharacter(id, owner,group,location,
            PLAYER_MOVE_SPEED, eCharacterGroup::Player, data->CharacterType())
        , _data(data)
        ,_dbThread(dbThread)
    {
        _attacks = playerAttack[data->CharacterType()];
    }

    void Player::GetCoin(char value)
    {
        auto getCoin = SendBuffer::Alloc();
        MakeGame_ResGetCoin(getCoin, ObjectId(), 1);
        _group.SendPacket(SessionId(), getCoin);
        
        _data->AddCoin(value);
        _dbThread->UpdateCoin(_data);
    }

    void Player::MakeCreatePacket(SendBuffer& buffer, bool spawn) const
    {
        ChatCharacter::MakeCreatePacket(buffer, spawn);
        MakeGame_ResPlayerDetail(buffer,ObjectId(),_data->Nick());
    }

    void Player::Die()
    {
        ChatCharacter::Die();
        
        auto die = SendBuffer::Alloc();
        MakeGame_ResDestroyActor(die, ObjectId(), true,1);
        _group.SendPacket(SessionId(), die);
    }
}