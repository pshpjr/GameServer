#pragma once
#include <string>

#include "Types.h"

namespace psh
{
    struct ServerInitData
    {
        std::string gameDBIP;
        Port gameDBPort;
        std::string gameDBID;
        std::string gameDBPwd;

        bool UseMonitorServer;
        String MonitorServerIP;
        Port MonitorServerPort;

        bool useMonsterAI;
    };
}

