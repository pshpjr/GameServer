#pragma once
#include <vector>
#include "../Base/Range.h"
#include "FVector.h"

struct AttackData
{
    psh::Range* Range;
    int Damage;
    psh::ObjectID Attacker;
    psh::eCharacterGroup AttackerType;
};


namespace psh
{
    struct ATTACK
    {
        inline static void Init()
        {
            playerAttack =
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

            monsterAttack =
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


        inline static std::vector<std::vector<pair<FVector, int>>> playerAttack;

        inline static std::vector<std::vector<pair<FVector, int>>> monsterAttack;


    };



  
}
