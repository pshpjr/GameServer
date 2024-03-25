#pragma once
#include <vector>
#include <array>
#include "Sector.h"

struct SEND_OFFSETS
{

    inline static void Init() {
        CreateTable =
        {
            {
                {{-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}
                , {{-1, 1}, {-1, 0}, {-1, -1}}
                , {{-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}}
            }
            , {
                    {{1, -1}, {0, -1}, {-1, -1}}
                , {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
                , {{-1, 1}, {0, 1}, {1, 1}}
            }
            , {
                    {{1, 1}, {1, 0}, {1, -1}, {1, 0}, {1, -1}}
                , {{1, 1}, {1, 0}, {1, -1}}
                , {{-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}}
            }
        };

        DeleteTable =
        {
            {
                {{-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}}
                , {{1, 1}, {1, 0}, {1, -1}}
                , {{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}}
            }
            , {
                {{-1, 1}, {0, 1}, {1, 1}}
                , {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}
                , {{1, -1}, {0, -1}, {-1, -1}}
            }
            , {
                {{1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}}
                , {{-1, -1}, {-1, 0}, {-1, 1}}
                , {{1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}
            }
            ,
        };
    }

    inline static std::vector<std::vector<std::vector<psh::Sector>>> CreateTable;

    inline static std::vector<std::vector<std::vector<psh::Sector>>> DeleteTable;

    //X,Y
    inline static std::array<psh::Sector, 1> Single = {{{0, 0}}};
    inline static constexpr std::array<psh::Sector, 9> BROADCAST = {
        {
            {1, -1}
            , {1, 0}
            , {1, 1}
            , {0, -1}
            , {0, 0}
            , {0, 1}
            , {-1, -1}
            , {-1, 0}
            , {-1, 1}
        }};
};
