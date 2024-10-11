#pragma once
#include "SessionTypes.h"

#include "ContentTypes.h"
#include "FVector.h"

namespace psh
{
    class Player;

    class DBData
    {
    public:
        DBData(const SessionID& sessionId
               , const AccountNo accountNo
               , const FVector& location
               , const char serverType
               , const char characterType
               , const int coin
               , const int hp
               , const Nickname& nick)
            : _sessionId(sessionId)
            , _accountNo(accountNo)
            , _location(location)
            , _serverType(serverType)
            , _characterType(characterType)
            , _coin(coin)
            , _hp(hp)
            , _nick(nick) {}

        void SessionId(const SessionID& session_id)
        {
            _sessionId = session_id;
        }

        void AccountNum(AccountNo account_no)
        {
            _accountNo = account_no;
        }

        void Location(const FVector& location)
        {
            _location = location;
        }

        void ServerType(char server_type)
        {
            _serverType = server_type;
        }

        void CharacterType(char character_type)
        {
            _characterType = character_type;
        }

        void Coin(int coin)
        {
            _coin = coin;
        }

        void Hp(int hp)
        {
            _hp = hp;
        }

        void Nick(const Nickname& nick)
        {
            _nick = nick;
        }

        [[nodiscard]] SessionID SessionId() const
        {
            return _sessionId;
        }

        [[nodiscard]] AccountNo AccountNum() const
        {
            return _accountNo;
        }

        [[nodiscard]] FVector Location() const
        {
            return _location;
        }

        [[nodiscard]] char ServerType() const
        {
            return _serverType;
        }

        [[nodiscard]] char CharacterType() const
        {
            return _characterType;
        }

        [[nodiscard]] int Coin() const
        {
            return _coin;
        }

        [[nodiscard]] int Hp() const
        {
            return _hp;
        }

        [[nodiscard]] Nickname Nick() const
        {
            return _nick;
        }

        void SaveAll(Player& player);
        void AddCoin(char value);

    private:
        SessionID _sessionId;
        AccountNo _accountNo;
        FVector _location;
        char _serverType;
        char _characterType;
        int _coin = 0;
        int _hp = 100;
        Nickname _nick;
    };
}
