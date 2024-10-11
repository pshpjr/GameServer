#pragma once
#include "ChatCharacter.h"

namespace psh
{
    class AiState;
    class MonsterAiComponent;

    //너무 하는게 없는데 상속 맞나?
    class Monster final : public ChatCharacter
    {
    public:
        Monster(Field& group, const GameObjectData& initData, std::unique_ptr<MonsterAiComponent> aiComponent);

        ~Monster() override = default;
        void MakeCreatePacket(SendBuffer& buffer, bool spawn) const override;
        void DieImpl() override;
    };
}
