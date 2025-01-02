#pragma once
#include <string>

#include "Types.h"

namespace psh
{
    //서버 시작시 필요한 정보들
    struct ServerInitData
    {
        std::string gameDBIP;
        Port gameDBPort;
        std::string gameDBID;
        std::string gameDBPwd;

        bool UseMonitorServer;
        String MonitorServerIP;
        Port MonitorServerPort;

        bool consoleMonitor;
    };
}

