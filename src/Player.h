#pragma once
#include <memory>

#include "ChatCharacter.h"
#include "ContentTypes.h"
#include "DBData.h"


//player는 어떤 값이 올바른지 모르니까 외부에서 보장해서 넣어야 하는가?
//player는 map에서만 생성?

namespace psh
{
    struct Sector;
    class Player;
    using PlayerRef = std::shared_ptr<Player>;
    class DBThreadWrapper;

    class Player final : public ChatCharacter
    {
    public:
        ~Player() override = default;

        Player(Field& group
            , const GameObjectData &initData
            , std::shared_ptr<DBData> dbData
            , DBThreadWrapper* dbThread);
        void GetCoin(char value) const;

        void SendCoinInfo() const;

        void MakeCreatePacket(SendBuffer& buffer, bool spawn) const override;
        void SendPacket(SendBuffer& buffer);

        [[nodiscard]] SessionID SessionId() const
        {
            return _data->SessionId();
        }

        void Die() override;


        [[nodiscard]] AccountNo AccountNumber() const
        {
            return _data->AccountNo();
        }

        std::shared_ptr<DBData> _data;
        void OnCreate() override;

    private:
        DBThreadWrapper* _dbThread = nullptr;
    };
}
