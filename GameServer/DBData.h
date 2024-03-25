#pragma once
#include "ContentTypes.h"
#include "FVector.h"
#include "Types.h"
#include "../Base/DBThreadWrapper.h"

namespace psh
{
    class DBData
    {
    public:
        DBData(const SessionID& sessionId
               , AccountNo accountNo
               , const FVector& location
               , char serverType
               , char characterType
               , int coin
               , int hp
               , Nickname nick)
            : _sessionId(sessionId)
            , _accountNo(accountNo)
            , _location(location)
            , _serverType(serverType)
            , _characterType(characterType)
            , _coin(coin)
            , _hp(hp)
            ,_nick(nick)
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
        void SetAccountNo(psh::AccountNo accountNo)
        {
            _accountNo = accountNo;
        }

        [[nodiscard]] FVector Location() const
        {
            return _location;
        }

        
        void SetLocation(FVector location)
        {
            _location = location;
        }

        [[nodiscard]] char ServerType() const
        {
            return _serverType;
        }

        void SetServerType(char serverType)
        {
            _serverType = serverType;
        }

        [[nodiscard]] char CharacterType() const
        {
            return _characterType;
        }

        void SetCharacterType(char characterType)
        {
            _characterType = characterType;
        }

        [[nodiscard]] int Coin() const
        {
            return _coin;
        }

        void SetCoin(int coin)
        {
            _coin = coin;
        }

        void AddCoin(int value)
        {
            _coin += value;
        }

        [[nodiscard]] int Hp() const
        {
            return _hp;
        }

        void SetHp(int hp)
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
