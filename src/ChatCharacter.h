#pragma once
#include "AttackData.h"
#include "GameObject.h"
#include "Timer.h"


namespace psh
{
    class AttackManager;

    class ChatCharacter : public GameObject {
        struct SkillAction {
            const SkillInfo skillInfo;
            Timer timer;
        };

    public:
        ChatCharacter(Field &group, const GameObjectData &initData)
            : GameObject(group, initData)
        {
            for (auto skill: ATTACK::GetSkillsByTemplate(initData.templateId))
            {
                _skills.emplace(skill, SkillAction{ATTACK::GetSkillInfoById(skill), {}});
            }
        }

        ~ChatCharacter() override = default;

        void MakeCreatePacket(SendBuffer &buffer, bool spawn) const override;

        void Attack(SkillID type, FVector dir);

        void Hit(DamageInfo info);

        bool isDead() const
        {
            return _hp <= 0;
        }

        void Revive()
        {
            _hp = 100;
        }

        void Die();

        /**
         * 주로 아이템 생성할 때 사용.
         * 죽은 이유 저장하기.
         */
        virtual void DieImpl()
        {
        };

    protected:
        int _hp = 100;
        HashMap<SkillID, SkillAction> _skills;
    };
}
