#pragma once
#include <string>

#include "Types.h"

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
