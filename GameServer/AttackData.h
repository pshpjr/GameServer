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
                    {{60, 60}, 4}
                    , {{60, 60}, 6}
                    , {{60, 60}, 8}
                    ,
                }
                , {
                    {{60, 60}, 4}
                    , {{60, 65}, 6}
                    , {{60, 60}, 8}
                    ,
                }
                , {
                    {{60, 60}, 5}
                    , {{60, 65}, 6}
                    , {{60, 60}, 7}
                    ,
                }
                , {
                    {{60, 60}, 5}
                    , {{60, 63}, 6}
                    , {{60, 61}, 7}
                    ,
                }
                ,
            };

            monsterAttack =
            {
                {
                    {{60, 60}, 1}
                    , {{60, 60}, 2}
                    , {{60, 60}, 3}
                    ,
                }
                , {
                    {{60, 60}, 1}
                    , {{60, 60}, 2}
                    , {{60, 60}, 3}
                    ,
                }
                , {
                    {{60, 60}, 8}
                    , {{60, 60},7}
                    , {{60, 60}, 6}
                    ,
                }
                , {
                    {{60, 60}, 6}
                    , {{60, 60}, 10}
                    , {{60, 60}, 4}
                    ,
                }
                ,
            };
        }


        inline static std::vector<std::vector<pair<FVector, int>>> playerAttack;

        inline static std::vector<std::vector<pair<FVector, int>>> monsterAttack;


    };



  
}
