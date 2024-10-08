//
// Created by pshpj on 24. 10. 7.
//

#include "MonsterSpawner.h"

#include "Field.h"
#include "Monster.h"

psh::MonsterSpawner::MonsterSpawner(ServerType type, Field &field): _type{type}
                                                                , _field{field}
{
}

void psh::MonsterSpawner::Update(int deltaTime)
{
    auto monsters = _field.GetMonsterCount();
    if (monsters < MAX_MONSTERS)
    {
        int toSpawn = MAX_MONSTERS - _field.GetMonsterCount();

        for (int i = 0; i < toSpawn; i++)
        {
            auto newData = _defaultData;
            newData.location = _field.GetRandomLocation();
            newData.templateId = 0;
            auto monster = std::make_shared<Monster>(_field, newData);
            monster->_selector = MonsterAi::PveTargetSelector;
            //TODO: 나중에 알맞은 타입으로 수정할 것
            _field.AddActor(monster);
        }
    }
}
