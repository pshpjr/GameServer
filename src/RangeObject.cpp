#include "RangeObject.h"

#include "Range.h"
#include "TableData.h"

psh::RangeObject::RangeObject(Field &group, const GameObjectData &initData, const float radius): GameObject(group, initData)
  , _range(std::make_shared<CircleRange>(initData.location, radius))
{
}

void psh::RangeObject::Enter(shared<GameObject> obj)
{
    _containObjects.push_back(obj);
    OnEnter(obj);
}

void psh::RangeObject::OnUpdate([[maybe_unused]] int delta)
{
    //가장 무식하게 짜기.
    //내 안에 있던 애들을 전부 순회하면서
    //벗어났다면
    //OnLeave를 호출한다.
    for (auto it = _containObjects.begin(); it != _containObjects.end(); ++it)
    {
        if (!Collision((*it)->Location()))
        {
            OnLeave(*it);
            it = _containObjects.erase(it);
        }
    }

    //범위 식별 가능한 영역들을 찾는다.
    //해당 섹터에 있는 모든 객체들을 순환한다.
    // 내 영역 안에 있다면
    // 이미 나에게 포함된 애인지 확인한다.
    // 그렇지 않다면
    // OnEnter를 호출한다.

    std::set<Sector> sectors;

    for(const auto point : _range->GetCoordinates())
    {
        sectors.insert(_map->GetSector(point));
    }

    //자기에게 맞는 적절한 객체만 선택하는 함수가 있어야 함.
    for(auto sector: sectors)
    {
        for(auto& p : _field.GetPlayerView(sector,SEND_OFFSETS::Single))
        {
            if(_range->Contains(p->Location()) && ranges::find(_containObjects,p) == _containObjects.end())
            {
                OnEnter(p);
                _containObjects.push_back(p);
            }
        }
    }
}


bool psh::RangeObject::Collision(const FVector point) const
{
    return _range->Contains(point);
}

void psh::RangeObject::OnDestroyImpl()
{
    for (auto it = _containObjects.begin(); it != _containObjects.end(); ++it)
    {
        if (!Collision((*it)->Location()))
        {
            OnLeave(*it);
            it = _containObjects.erase(it);
        }
    }
}

psh::SingleInteractionObject::SingleInteractionObject(Field &group, const GameObjectData &initData, PoolPtr<Range> range): GameObject(group, initData)
  , _range(move(range))
{

}

void psh::SingleInteractionObject::Enter(shared<GameObject> obj)
{
    OnEnter(obj);
}

void psh::SingleInteractionObject::OnUpdate(int delta)
{
    std::set<Sector> sectors;

    for(const auto point : _range->GetCoordinates())
    {
        sectors.insert(_map->GetSector(point));
    }

    //자기에게 맞는 적절한 객체만 선택하는 함수가 있어야 함.
    for(auto sector: sectors)
    {
        for(auto& p : _field.GetPlayerView(sector,SEND_OFFSETS::Single))
        {
            if(_range->Contains(p->Location()))
            {
                OnEnter(p);
                _field.DestroyActor(shared_from_this());
                return;
            }
        }
    }
}

psh::Item::Item(Field &group, const GameObjectData &initData, PoolPtr<Range> range): SingleInteractionObject{group,initData,move(range)}
{

}

void psh::Item::OnEnter(shared<GameObject> &obj)
{

}

