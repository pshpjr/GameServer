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
            monster->SetAI(std::move(ai));

            _field.AddActor(monster);
        }
    }
}

std::unique_ptr<psh::MonsterSpawner> psh::MonsterSpawner::GetSpawner(ServerType type, Field& field
                                                                     , GameMap<ObjectID, shared<GameObject>>&
                                                                     monsterMap)
{
    //이 함수에서만 생성자 호출 가능.
#pragma warning(push)
#pragma warning(disable : 4996)
    switch (type)
    {
    case ServerType::Village:
        return nullptr;
        break;
    case ServerType::Easy:
        return std::make_unique<MonsterSpawner>(ServerType::Easy, field, monsterMap, MonsterAi::PveTargetSelector);
        break;
    case ServerType::Hard:
        return std::make_unique<MonsterSpawner>(ServerType::Hard, field, monsterMap, MonsterAi::PveTargetSelector);
        break;
    case ServerType::Pvp:
        return std::make_unique<MonsterSpawner>(ServerType::Pvp, field, monsterMap, MonsterAi::PvpTargetSelector);
        break;
    default:
        ASSERT_CRASH(false, "Invalid ServerType");
        break;
    }
    return nullptr;
#pragma warning(pop)
}
