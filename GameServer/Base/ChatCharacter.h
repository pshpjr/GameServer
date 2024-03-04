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
        void Attack(char type);
        void Hit(int damage,const shared_ptr<ChatCharacter>& attacker);
        void OnUpdate(float delta) override{}
        void MoveStart(FVector destination) override;

    protected:
        void OnMove() override;

    public:
        virtual void Die();
    protected:
        int _hp = 100;

        vector<pair<FVector,int>> _attacks;
    };
}

