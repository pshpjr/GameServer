#define PROFILE

#include "AttackData.h"
#include "BuildVersion.h"
#include "CloseSignalHandler.h"
#include "CrashDump.h"
#include "DBConnection.h"
#include "ExceptionTrace.h"
#include "Server.h"
#include "TableData.h"

#include <cstdlib>
// auto& manager = ProfileManager::Get();

int main()
{
    int exitCode = EXIT_SUCCESS;
    psh::util::RunWithTrace("CMakeServer main", [&]
    {
        CrashDump::SetBuildInfo(BuildVersion::Metadata);
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
            exitCode = EXIT_FAILURE;
            return;
        }

        server->SetDefaultTimeout(0);
        fmt::println("Start");
        server->Start();

        server->Wait();
    });
    return exitCode;
}


