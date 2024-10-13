#define PROFILE

#include "AttackData.h"
#include "DBConnection.h"
#include "ProcessMonitor.h"
#include "Server.h"
#include "TableData.h"
#include "optick.config.h"
// auto& manager = ProfileManager::Get();


int main()
{
    ProcessMonitor monitor{L"Google Chrome"};

    monitor.Update();
    try
    {
        DBConnection::LibInit();
        SEND_OFFSETS::Init();
        psh::ATTACK::Init();

        const auto server = std::make_unique<psh::Server>();
        psh::Server::SetDisableClickAndClose();
        server->Start();

        server->Wait();
    }
    catch (const std::exception& e)
    {
        std::cout << "Unhandled Exception : " << e.what() << '\n';

        std::cin.get();
    }

    return 0;
}


