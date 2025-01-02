#pragma once

#include <functional>
#include "AttackData.h"


namespace psh
{
    // 전방 선언(forward declarations)
    class Field;
    struct AttackInfo;
    enum class ServerType : unsigned char;

    // 클래스 내부의 타입 정의 및 상수
    enum class AttackResult
    {
        Success
        , Invalid
        , Miss
        ,
    };

    namespace victim_select
    {
        //공격 대상 찾는 함수. pvp, pve에 따라 공격 대상이 달라짐.
        // bind해서 첫 인자 고정해서 사용할 것.
        using VictimSelectFunction = std::function<AttackResult(Field& field, const AttackInfo&)>;

        // 공용 멤버 함수
        VictimSelectFunction GetVictimByServerType(ServerType type);
    }
} // namespace psh::victim_select
