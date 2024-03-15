#pragma once
#include "Container.h"

class GameDataLoader
{
public:
    virtual void Get(String name) = 0;
    virtual ~GameDataLoader() = default;
};

class TmpDataLoader : public GameDataLoader
{
public:
    TmpDataLoader()
    {
    }

private:
    HashMap<String, int> _data;
};
