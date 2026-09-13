//
// Created by pshpj on 24. 10. 7.
//

#include "MonsterSpawner.h"

#include <utility>

#include "AiComponent.h"
#include "Field.h"
#include "Monster.h"
#include "Rand.h"

psh::MonsterSpawner::MonsterSpawner(ServerType type, Field& field
                                    , GameMap<ObjectID, shared<GameObject>>& monsterMap
                                    , MonsterAi::TargetSelector selector)
    : _type{type}
    , _field{field}
    , _monsterMap{monsterMap}
    , _selector{std::move(selector)}
    , _mapSize{static_cast<float>(_monsterMap.GetMapSize())} {}

void psh::MonsterSpawner::Update(int deltaTime)
{
    auto monsters = _monsterMap.GetObjectCount();
    if (monsters < MAX_MONSTERS)
    {
        int toSpawn = MAX_MONSTERS - monsters;

        for (int i = 0; i < toSpawn; i++)
        {
            auto newData = _defaultData;
            newData.location = FVector{psh::RandomUtil::Rand(0.0, _mapSize), psh::RandomUtil::Rand(0.0, _mapSize)};

            int monsterTypeBase = static_cast<char>(_type) * 4;
            newData.templateId = RandomUtil::Rand(monsterTypeBase, monsterTypeBase + 3);
            auto monster = std::make_shared<Monster>(_field, newData, nullptr);
            auto ai = std::make_unique<MonsterAiComponent>(*monster, _selector);
            monster->SetAi(std::move(ai));

            _field.AddActor(monster);
        }
    }
}

std::unique_ptr<psh::MonsterSpawner> psh::MonsterSpawner::GetSpawner(ServerType type, Field& field
                                                                     , GameMap<ObjectID, shared<GameObject>>&
                                                                     monsterMap)
{
    //이 함수에서만 몬스터 스포너 생성자 호출 가능.
#pragma warning(push)
#pragma warning(disable : 4996)

    auto selector = MonsterAi::GetTargetSelectorByType(type);
    switch (type)
    {
    case ServerType::Village:
        return nullptr;
        break;
    case ServerType::Easy:
        [[fallthrough]];
    case ServerType::Hard:
        [[fallthrough]];
    case ServerType::Pvp:
        return std::make_unique<MonsterSpawner>(type, field, monsterMap, selector);
        break;
    default:
        ASSERT_CRASH(false, "Invalid ServerType");
        break;
    }
    return nullptr;
#pragma warning(pop)
}
