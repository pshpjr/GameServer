#include "Player.h"

#include <PacketGenerated.h>

#include <utility>

#include "AiComponent.h" //상위 클래스에서 쓰기 때문에 소멸자 호출 시 필요함.
#include "DBThreadWrapper.h"
#include "Field.h"
#include "GroupManager.h"
#include "TableData.h"

namespace psh
{
    constexpr float PLAYER_MOVE_SPEED = 200;

    Player::~Player() = default;

    Player::Player(Field& group
                   , const GameObjectData& initData
                   , std::shared_ptr<DBData> dbData
                   , DBThreadWrapper* dbThread)
        : ChatCharacter(group, initData, nullptr)
        , _data(std::move(dbData))
        , _nextGroup{None}
        , _dbThread(dbThread) {}

    void Player::AddCoin(const char value) const
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

    void Player::DieImpl() {}

    void Player::UpdateDBData()
    {
        _data->SaveAll(*this);
    }

    inline void Player::OnCreateImpl()
    {
        //db에 갱신된 정보 저장하고
        _dbThread->EnterGroup(_data);

        //주위 객체들 정보 받아옴.
        auto view = _field.GetObjectView(Field::ViewObjectType::All, Location(), SEND_OFFSETS::BROADCAST);
        for (const auto& obj : view)
        {
            if (obj.get() == this)
            {
                continue;
            }
            auto otherObjectPacket = SendBuffer::Alloc();
            obj->MakeCreatePacket(otherObjectPacket, false);
            SendPacket(otherObjectPacket);
        }

        SendCoinInfo();
    }

    void Player::OnDestroyImpl()
    {
        UpdateDBData();
        if (_removeReason == removeReason::GroupChange)
        {
            _dbThread->SaveAll(_data, [this] {
                _field.MoveField(_data->SessionId(), _nextGroup);
            });

            return;
        }

        _dbThread->SaveAll(_data, nullptr);
    }
}
