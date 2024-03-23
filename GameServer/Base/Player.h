#pragma once
#include <memory>

#include "ContentTypes.h"
#include "ChatCharacter.h"
#include "../Data/DBData.h"


//player는 어떤 값이 올바른지 모르니까 외부에서 올바른 값으로 넣어줘야 하는가?
//player는 map에서만 생성하는가?

namespace psh
{
    struct Sector;

    class Player : public ChatCharacter
    {
    public:
        ~Player() override = default;

    public:
        Player(ObjectID id
            , ObjectManager& owner
            , GroupCommon& group
                            , FVector location
                            , shared_ptr<DBData> data
                            , DBThreadWrapper* dbThread);

        void GetCoin(char value);
        
        void MakeCreatePacket(SendBuffer& buffer, bool spawn) const override;
        
        [[nodiscard]] SessionID SessionId() const
        {
            return _data->SessionId();
        }
        void Die() override;


        [[nodiscard]] AccountNo AccountNumber() const
        {
            return _data->AccountNo();
        }
        shared_ptr<DBData> _data;
    private:

        DBThreadWrapper* _dbThread= nullptr;
    };
}
