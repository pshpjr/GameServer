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
                            , float moveSpeedPerSec = 400
                            , eCharacterGroup characterType = eCharacterGroup::Object
                            , char type = 0):
                                      GameObject(id,owner,group, location, {0,0}, moveSpeedPerSec, characterType, type)
        {
        }

        ~ChatCharacter() override = default;
        void Attack(char type);
        void Hit(int damage, const ObjectID attacker);
        bool isDead() const
        {
            return _dead;
        }

        void Revive()
        {
            _dead = false;
            _hp = 100;
        }
        void SetAttackManager(AttackManager* manager){_attackManager = manager;}
        void Update(int delta) override;

        void Die();

    protected:
        int _hp = 100;
        bool _dead = false;
        vector<pair<FVector, int>> _attacks;
    };
}
