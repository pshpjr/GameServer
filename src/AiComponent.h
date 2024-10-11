//
// Created by pshpj on 24. 10. 10.
//

#ifndef MONSTERAICOMPONENT_H
#define MONSTERAICOMPONENT_H
#include "MonsterAi.h"
#include "Timer.h"
#include "Updatable.h"

namespace psh
{
    class ChatCharacter;

    class MonsterAiComponent : public Updatable
    {
        const float MAX_MOVE_RADIUS = 1000;
        int ACTION_DELAY{1000};

    public:
        MonsterAiComponent(ChatCharacter& owner, MonsterAi::TargetSelector selector);

        ~MonsterAiComponent() override;
        bool IsReturning();
        void Update(int delta) override;

    private:
        ChatCharacter* _owner{nullptr};
        Timer _searchDelay;
        Timer _moveDelay;
        std::weak_ptr<GameObject> _target;
        MonsterAi::TargetSelector _selector;
        FVector _spawnLocation;
        int _curAttackIndex{0};
        bool _returning{};
        std::vector<SkillID> _skills;
    };
}


#endif //MONSTERAICOMPONENT_H
