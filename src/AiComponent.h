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

    //게임 캐릭터를 조종할 수 있는 ai 컴포넌트.
    class MonsterAiComponent : public Updatable
    {
        const float MAX_MOVE_RADIUS = 1000;
        int ACTION_DELAY{1000};

    public:
        MonsterAiComponent(ChatCharacter& owner, MonsterAi::TargetSelector selector);

        ~MonsterAiComponent() override;

        void Update(int delta) override;

    private:
        //스폰 지점 복귀 중인지
        bool IsReturning();

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
