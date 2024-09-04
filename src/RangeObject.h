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
    class RangeObject : public GameObject
    {
    public:
        RangeObject(Field& group, const GameObjectData& initData, const float radius);

        virtual void OnEnter(shared<GameObject>& obj) = 0;
        virtual void OnLeave(shared<GameObject>& obj) = 0;
        void Enter(shared<GameObject> obj);
        bool Collision(FVector point) const;

    protected:
        void OnDestroy() override;
        void OnUpdate(int delta) final;

    private:
        shared<CircleRange> _range;
        std::list<shared<GameObject>> _containObjects;
    };

    class SingleInteractionObject : public GameObject
    {
    public :
        SingleInteractionObject(Field& group, const GameObjectData& initData, PoolPtr<Range> range);

        void Enter(shared<GameObject> obj);

    protected:
        virtual void OnEnter(shared<GameObject>& obj) = 0;

    private:
        void OnUpdate(int delta) final;

    private:
        PoolPtr<Range> _range;
    };

    class Item : public SingleInteractionObject
    {
    public :
        Item(Field& group, const GameObjectData& initData, PoolPtr<Range> range);

    protected:
        void OnEnter(shared<GameObject>& obj) final;

    private:
        PoolPtr<Range> _range;
    };
}
