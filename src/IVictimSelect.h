#pragma once

#include <functional>
#include "AttackData.h"

namespace psh::victim_select
{
    // 전방 선언(forward declarations)
    class Field;
    class AttackInfo;
    enum class ServerType;

    // 클래스 내부의 타입 정의 및 상수
    enum class AttackResult {
        Success, Invalid, Miss,
    };

    // bind해서 첫 인자 고정해서 사용할 것.
    using VictimSelectFunction = std::function<AttackResult(psh::Field &field, const psh::AttackInfo &)>;

    // 공용 멤버 함수
    VictimSelectFunction GetVictimByServerType(psh::ServerType type);
} // namespace psh::victim_select
