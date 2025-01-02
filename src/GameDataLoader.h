#pragma once
#include "Container.h"

//DB나 파일에서 데이터 읽어올 수 있지 않을까? 해서 추가해둔 클래스.
class GameDataLoader
{
public:
    virtual void Get(String name) = 0;
    virtual ~GameDataLoader() = default;
};

class TmpDataLoader : public GameDataLoader
{
public:
    TmpDataLoader() = default;

private:
    HashMap<String, int> _data;
};
