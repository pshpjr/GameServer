#pragma once
#include "GameObject.h"

namespace psh
{
    class ChatCharacter : public psh::GameObject
    {
    public:
        ChatCharacter(ObjectID id, FVector location, FVector direction, float moveSpeed,eCharacterGroup group = eCharacterGroup::Object, char type = 0):
        GameObject( id,location, direction,moveSpeed,group,type)
        {
        }
        
        ~ChatCharacter() override = default;
        virtual void Attack(char type){}
        virtual void Hit(int damage){}
        void OnUpdate(float delta) override{}
        virtual void Die() {}
        virtual void SendPacket(IOCP* iocp, SendBuffer& buffer){};
    protected:
        int _hp = 100;
    };
}

