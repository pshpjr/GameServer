#include "stdafx.h"
#include "Player.h"

#include <PacketGenerated.h>

#include "IOCP.h"
#include "ObjectManager.h"

#include "GameMap.h"
#include "AttackData.h"


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
        _attacks = ATTACK::playerAttack[data->CharacterType()];
    }

    void Player::GetCoin(char value)
    {
        _data->AddCoin(value);
        _dbThread->UpdateCoin(_data);

        SendCoinInfo();
    }

    void Player::SendCoinInfo()
    {
        auto getCoin = SendBuffer::Alloc();
        MakeGame_ResGetCoin(getCoin, ObjectId(), _data->Coin());
        _group.SendPacket(SessionId(), getCoin);
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
