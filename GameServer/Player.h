#pragma once
#include <memory>

#include "ContentTypes.h"
#include "ChatCharacter.h"
#include "DBData.h"


//player�� � ���� �ùٸ��� �𸣴ϱ� �ܺο��� �ùٸ� ������ �־���� �ϴ°�?
//player�� map������ �����ϴ°�?

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

        void SendCoinInfo();
        
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
        void OnCreate() override
        {
            SendCoinInfo();
        }

    private:

        DBThreadWrapper* _dbThread= nullptr;
    };

    
}
