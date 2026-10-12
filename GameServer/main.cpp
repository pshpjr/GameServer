#define PROFILE

#include "AttackData.h"
#include "BuildVersion.h"
#include "CloseSignalHandler.h"
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
        psh::Server* serverpt = nullptr;
        psh::util::CloseSignalListener listener(
            [&serverpt]()
            {
                fmt::println("Close signal received");
                if (serverpt)
                    serverpt->Stop();
            });


        DBConnection::LibInit();
        SEND_OFFSETS::Init();
        psh::ATTACK::Init();

        const auto server = std::make_unique<psh::Server>();
        serverpt = server.get();

        if (!server->Init())
        {
            std::cerr << "Failed to initialize the game server." << std::endl;
            return EXIT_FAILURE;
        }

        server->SetDefaultTimeout(0);
        fmt::println("Start");
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


