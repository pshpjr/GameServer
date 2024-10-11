﻿#pragma once
#include "AttackData.h"
#include "GameObject.h"
#include "Timer.h"


namespace psh
{
    class AttackManager;
    class MoveComponent;
    class MonsterAiComponent;

    class ChatCharacter : public GameObject
    {
        friend class MonsterAiComponent;

        struct SkillCooldown
        {
            const SkillInfo skillInfo;
            Timer timer;
        };

    public:
        ChatCharacter(Field& group, const GameObjectData& initData, std::unique_ptr<MonsterAiComponent> aiComponent);

        ~ChatCharacter() override;

        void OnUpdate(int delta) override;

        //이동 관련
        void MoveStart(FVector destination) const;

        void MoveStop() const;

        bool IsMoving() const;

        [[nodiscard]] MoveComponent& GetMovable() const;

        void Attack(SkillID type, FVector dir);

        void Hit(DamageInfo info);

        bool isDead() const;

        void Revive();

        void Die();

        /**
        * 주로 아이템 생성할 때 사용.
        * 죽은 이유 저장하기.
        */
        virtual void DieImpl() {};
        int Hp();

        void MakeCreatePacket(SendBuffer& buffer, bool spawn) const override;
        void SetAI(unique<MonsterAiComponent> component);

    protected:
        int _hp = 100;
        HashMap<SkillID, SkillCooldown> _skills;
        unique<MoveComponent> _movementComponent;
        unique<MonsterAiComponent> _aiComponent;
    };
}
