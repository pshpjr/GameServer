#pragma once
#include <vector>

#include "FVector.h"

namespace psh
{
    inline static std::vector<std::vector<pair<FVector, int>>> playerAttack =
    {
        {
            {{20, 30}, 5}
            , {{20, 30}, 10}
            , {{20, 30}, 15}
            ,
        }
        , {
            {{20, 30}, 10}
            , {{10, 35}, 15}
            , {{5, 40}, 20}
            ,
        }
        , {
            {{30, 30}, 7}
            , {{35, 35}, 8}
            , {{40, 40}, 9}
            ,
        }
        , {
            {{20, 30}, 10}
            , {{20, 33}, 15}
            , {{40, 31}, 20}
            ,
        }
        ,
    };

    inline static std::vector<std::vector<pair<FVector, int>>> monsterAttack =
    {
        {
            {{20, 20}, 10}
            , {{20, 20}, 20}
            , {{40, 20}, 30}
            ,
        }
        , {
            {{20, 20}, 10}
            , {{20, 20}, 2}
            , {{40, 20}, 3}
            ,
        }
        , {
            {{20, 20}, 10}
            , {{20, 20}, 2}
            , {{40, 20}, 3}
            ,
        }
        , {
            {{20, 20}, 10}
            , {{20, 20}, 2}
            , {{40, 20}, 3}
            ,
        }
        ,
    };
}
