//
// Created by pshpj on 24. 10. 7.
//

#ifndef MONSTERSPAWNER_H
#define MONSTERSPAWNER_H

#include <ContentTypes.h>
#include "Field.h"
#include "GameObject.h"
#include "MonsterAi.h"

namespace psh
{
    // 전방 선언 (forward declarations)
    class Field;

    /**
     * 몬스터를 스폰하는 클래스
     */
    class MonsterSpawner : public Updatable {
    public:
        [[deprecated("Use GetSpawner func instead")]]
        MonsterSpawner(ServerType type, Field &field, GameMap<ObjectID, shared<Monster> > &monsterMap
                     , MonsterAi::TargetSelector selector);

        // 공용 (public) 멤버 함수
        /**
         * 주기적으로 몬스터를 스폰합니다.
         * @param deltaTime Delta 시간
         */
        void Update(int deltaTime) override;

        /**
         * 특정 서버 타입에 맞는 스포너를 반환합니다.
         * @param type 서버 타입
         * @param field 필드 객체
         * @param monsterMap 몬스터 맵
         * @return 맞춤형 MonsterSpawner 객체
         */
        static std::unique_ptr<MonsterSpawner> GetSpawner(ServerType type, Field &field
                                                        , GameMap<ObjectID, shared<Monster> > &monsterMap);

        ~MonsterSpawner() final = default;

    private:
        // 클래스 내부의 타입 정의 및 상수
        const int MAX_MONSTERS = 30;
        const float _mapSize;

        // 멤버 변수
        ServerType _type;
        Field &_field;
        GameMap<ObjectID, shared<Monster> > &_monsterMap;
        MonsterAi::TargetSelector _selector;

        // 기본 몬스터 데이터
        GameObjectData _defaultData = {
            {0, 0}, {0, 0}, 200, eObjectType::Monster, 50
        };
    };
} // namespace psh

#endif // MONSTERSPAWNER_H
