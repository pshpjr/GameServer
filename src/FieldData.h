#pragma once
#include <ContentTypes.h>
#include <memory>

#include "IVictimSelect.h"


//어떤 필드 타입을 요청했을 때 그에 맞는 크기, 공격 방식등을 리턴해줌.
struct FieldData
{
    float size = 6400;
    float sectorSize = 400;
    std::unique_ptr<psh::IVictimSelect> attackStrategy;
};

static FieldData GetFieldDataByType(const psh::ServerType type)
{
    FieldData ret;
    ret.size = 6400;
    ret.sectorSize = 400;

    switch (type)
    {
    case psh::ServerType::Village:
        ret.attackStrategy = nullptr;
        break;
    case psh::ServerType::Easy:
        break;
    case psh::ServerType::Hard:
        break;
    case psh::ServerType::Pvp:
        ret.attackStrategy = std::make_unique<psh::PveVictim>();
        break;
    case psh::ServerType::End:
        ASSERT_CRASH(false, "Invalid State");
        break;
    }

    return ret;

}


//나중에 필드 
