#pragma once
#include "Field.h"
#include "GameObject.h"

namespace psh
{
    class Range;
    class CircleRange;
}

namespace psh
{
    //외부에서 RangeObject에게 특정 obj가 올라갔다고 알려줘야 함. 
    //rangeObject의 개수가 더 적을 것이기 때문.
    class RangeObject : public GameObject {
    public:
        RangeObject(Field &group, const GameObjectData &initData, float radius);

        virtual void OnEnter(shared<GameObject> &obj) = 0;

        virtual void OnLeave(shared<GameObject> &obj) = 0;

        void Enter(shared<GameObject> obj);

        bool Collision(FVector point) const;

    protected:
        void OnDestroyImpl() override;

        void Update(int delta) final;

    private:
        shared<CircleRange> _range;
        std::list<shared<GameObject> > _containObjects;
    };

    //지금은 플레이어랑만 상호작용 함.
    class SingleInteractionObject : public GameObject {
    public :
        SingleInteractionObject(Field &group, const GameObjectData &initData, PoolPtr<Range> range);

        void Enter(shared<GameObject> obj);

    protected:
        virtual void OnEnter(const shared<GameObject> &obj) = 0;

    private:
        void Update(int delta) final;

        PoolPtr<Range> _range;
    };

    class Item : public SingleInteractionObject {
    public :
        Item(Field &group, const GameObjectData &initData, PoolPtr<Range> range);

    protected:
        void OnEnter(const shared<GameObject> &obj) final;

    private:
        PoolPtr<Range> _range;
    };
}
