#define PROFILE

#include "AttackData.h"
#include "BuildVersion.h"
#include "CloseSignalHandler.h"
#include "CrashDump.h"
#include "DBConnection.h"
#include "ExceptionTrace.h"
#include "Metrics.h"
#include "Server.h"
#include "TableData.h"

#include <atomic>
#include <cstdlib>
// auto& manager = ProfileManager::Get();

int main()
{
    int exitCode = EXIT_SUCCESS;
    psh::util::RunWithTrace("CMakeServer main", [&]
    {
        CrashDump::SetBuildInfo(BuildVersion::Metadata);
        std::atomic<psh::Server*> serverpt{nullptr};
        std::unique_ptr<psh::Server> server;
        psh::util::CloseSignalListener listener(
            [&serverpt]()
            {
                fmt::println("Close signal received");
                if (auto* target = serverpt.load())
                    target->Stop();
            });


        DBConnection::LibInit();
        SEND_OFFSETS::Init();
        psh::ATTACK::Init();

        server = std::make_unique<psh::Server>();
        serverpt.store(server.get());

        auto metricsConfig = psh::metrics::ConfigFromEnv("gameserver");
        metricsConfig.useConsoleMonitor = server->UseConsoleMonitor();
        if (auto result = psh::metrics::Init(metricsConfig); !result)
            fmt::println(stderr, "metrics disabled: {}", result.Error());

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
        psh::metrics::Shutdown();
    });
    return exitCode;
}


