#define PROFILE

#include "AttackData.h"
#include "BuildVersion.h"
#include "CrashDump.h"
#include "DBConnection.h"
#include "Server.h"
#include "TableData.h"
#include <cstdlib>
// auto& manager = ProfileManager::Get();


int main()
{
    CrashDump::SetBuildInfo(BuildVersion::Metadata);
    try
    {
        DBConnection::LibInit();
        SEND_OFFSETS::Init();
        psh::ATTACK::Init();

        const auto server = std::make_unique<psh::Server>();
        if (!server->Init())
        {
            std::cerr << "Failed to initialize the game server." << std::endl;
            return EXIT_FAILURE;
        }
        psh::Server::SetDisableClickAndClose();
        server->SetDefaultTimeout(0);



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


