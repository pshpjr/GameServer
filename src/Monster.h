#pragma once
#include "ChatCharacter.h"
#include "MonsterAi.h"

namespace psh
{
    class AiState;

    class Monster final : public ChatCharacter {
    public:
        Monster(Field &group, const GameObjectData &initData);

        ~Monster() override = default;

        void OnUpdate(int delta) override;

        void OnDestroyImpl() override;

        void DieImpl() override;

        FVector _spawnLocation = {0, 0};
        int attackCooldown = 0;
        int moveCooldown = 0;
        int searchCooldown = 0;
        std::weak_ptr<ChatCharacter> _target;
        MonsterAi::TargetSelector _selector;

        int attackRange{0};
    };
}
