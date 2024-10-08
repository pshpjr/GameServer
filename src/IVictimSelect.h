#pragma once
#include <functional>

#include "AttackData.h"

//공격 대상 관련 처리.
//

namespace psh::victim_select
{
    //bind해서 첫 인자 고정해서 사용할 것.
    using VictimSelectFunction = std::function<void(psh::Field &field, const psh::AttackInfo &)>;

    VictimSelectFunction GetVictimByServerType(psh::ServerType type);
}
