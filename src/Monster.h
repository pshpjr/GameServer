#pragma once
#include "ChatCharacter.h"

namespace psh
{
    class AiState;

    class Monster final : public ChatCharacter
    {
    public:
        Monster(Field& group, GameObjectData& initData);
        ~Monster() override = default;

        void OnUpdate(int delta) override;

        void OnDestroy() override;

        FVector _spawnLocation = {0, 0};
        int attackCooldown = 0;
        int moveCooldown = 0;
        int searchCooldown = 0;
        std::weak_ptr<ChatCharacter> _target;


        int attackRange{0};
    };
}
