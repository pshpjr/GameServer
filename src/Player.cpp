#include "Player.h"

#include <PacketGenerated.h>

#include <utility>

#include "DBThreadWrapper.h"
#include "Field.h"
#include "TableData.h"

namespace psh
{
    constexpr float PLAYER_MOVE_SPEED = 200;

    Player::Player(Field& group
        , const GameObjectData &initData
        , std::shared_ptr<DBData> dbData
        , DBThreadWrapper* dbThread)
        : ChatCharacter(group, initData)
        , _data(std::move(dbData))
        , _dbThread(dbThread)
    {
        //_skills = ATTACK::playerAttack[dbData->CharacterType()];
    }

    void Player::GetCoin(const char value) const
    {
        _data->AddCoin(value);
        _dbThread->UpdateCoin(_data);

        SendCoinInfo();
    }

    void Player::SendCoinInfo() const
    {
        auto getCoin = SendBuffer::Alloc();
        MakeGame_ResGetCoin(getCoin, ObjectId(), _data->Coin());
        _field.SendPacket(SessionId(), getCoin);
    }

    void Player::MakeCreatePacket(SendBuffer& buffer, const bool spawn) const
    {
        ChatCharacter::MakeCreatePacket(buffer, spawn);
        MakeGame_ResPlayerDetail(buffer, ObjectId(), _data->Nick());
    }

    void Player::SendPacket(const SendBuffer& buffer) const
    {
        _field.SendPacket(SessionId(), buffer);
    }

    void Player::Die()
    {
        ChatCharacter::Die();

        auto die = SendBuffer::Alloc();
        MakeGame_ResDestroyActor(die, ObjectId(), true, 1);
        _field.SendPacket(SessionId(), die);
    }

    inline void Player::OnCreate()
    {
        for (auto otherObjects = _field.GetObjectView(Location(), SEND_OFFSETS::BROADCAST);
             auto& obj : otherObjects)
        {
            if (obj.get() == this)
            {
                continue;
            }
            auto otherObjectPacket = SendBuffer::Alloc();

            obj->MakeCreatePacket(otherObjectPacket, false);
            _field.SendPacket(SessionId(), otherObjectPacket);
        }

        SendCoinInfo();
    }
}
