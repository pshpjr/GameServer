#pragma once
#include "SessionTypes.h"

#include "ContentTypes.h"
#include "FVector.h"

namespace psh
{
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
            , _nick(nick)
        {
        }

        [[nodiscard]] SessionID SessionId() const
        {
            return _sessionId;
        }

        void SetSessionId(const SessionID& sessionId)
        {
            _sessionId = sessionId;
        }

        [[nodiscard]] AccountNo AccountNo() const
        {
            return _accountNo;
        }

        [[nodiscard]] Nickname Nick() const
        {
            return _nick;
        }

        void SetAccountNo(const psh::AccountNo accountNo)
        {
            _accountNo = accountNo;
        }

        [[nodiscard]] FVector Location() const
        {
            return _location;
        }


        void SetLocation(const FVector location)
        {
            _location = location;
        }

        [[nodiscard]] char ServerType() const
        {
            return _serverType;
        }

        void SetServerType(const char serverType)
        {
            _serverType = serverType;
        }

        [[nodiscard]] char CharacterType() const
        {
            return _characterType;
        }

        void SetCharacterType(const char characterType)
        {
            _characterType = characterType;
        }

        [[nodiscard]] int Coin() const
        {
            return _coin;
        }

        void SetCoin(const int coin)
        {
            _coin = coin;
        }

        void AddCoin(const int value)
        {
            _coin += value;
        }

        [[nodiscard]] int Hp() const
        {
            return _hp;
        }

        void SetHp(const int hp)
        {
            _hp = hp;
        }

    private:
        SessionID _sessionId;
        psh::AccountNo _accountNo;
        FVector _location;
        char _serverType;
        char _characterType;
        int _coin = 0;
        int _hp = 100;
        Nickname _nick;
    };
}
