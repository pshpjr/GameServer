#pragma once
#include "AttackManager.h"
#include "GameObject.h"


namespace psh
{
    class AttackManager;

    class ChatCharacter : public GameObject
    {
    public:
        ChatCharacter(ObjectID id
, ObjectManager& owner
, GroupCommon& group
                            , FVector location
                            , float moveSpeedPerSec = 200
                            , eCharacterGroup characterType = eCharacterGroup::Object
                            , char type = 0):
                                      GameObject(id,owner,group, location, {0,0}, moveSpeedPerSec, characterType, type)
        {
        }

        ~ChatCharacter() override = default;
        void MakeCreatePacket(SendBuffer& buffer, bool spawn) const override;
        void Attack(char type);
        void Hit(int damage, const ObjectID attacker);
        bool isDead() const
        {
            return _hp <= 0;
        }

        void Revive()
        {
            _hp = 100;
            SetNeedUpdate(true);
        }
        void SetAttackManager(AttackManager* manager){_attackManager = manager;}
        virtual void Die();

    protected:
        int _hp = 100;
        vector<pair<FVector, int>> _attacks;
    };
}
