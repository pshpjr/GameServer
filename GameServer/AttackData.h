#pragma once
#include <vector>
#include "Range.h"
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
                    {{20, 30}, 50}
                    , {{20, 30}, 10}
                    , {{20, 30}, 15}
                    ,
                }
                , {
                    {{20, 30}, 100}
                    , {{10, 35}, 15}
                    , {{5, 40}, 20}
                    ,
                }
                , {
                    {{30, 30}, 70}
                    , {{35, 35}, 8}
                    , {{40, 40}, 9}
                    ,
                }
                , {
                    {{20, 30}, 100}
                    , {{20, 33}, 15}
                    , {{40, 31}, 20}
                    ,
                }
                ,
            };

            monsterAttack =
            {
                {
                    {{20, 20}, 1}
                    , {{20, 20}, 2}
                    , {{40, 20}, 3}
                    ,
                }
                , {
                    {{20, 20}, 1}
                    , {{20, 20}, 2}
                    , {{40, 20}, 3}
                    ,
                }
                , {
                    {{20, 20}, 1}
                    , {{20, 20}, 2}
                    , {{40, 20}, 3}
                    ,
                }
                , {
                    {{20, 20}, 1}
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
