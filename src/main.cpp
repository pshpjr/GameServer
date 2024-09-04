#define PROFILE
#include <Profiler.h>

#include "AttackData.h"
#include "DBConnection.h"
#include "ModernObjectPool.h"
#include "Server.h"
#include "Range.h"
#include "TableData.h"

// auto& manager = ProfileManager::Get();



int main()
{
    try
    {
        DBConnection::LibInit();
        SEND_OFFSETS::Init();
        psh::ATTACK::Init();

        const auto server = std::make_unique<psh::Server>();
        server->SetDisableClickAndClose();
        server->Start();
        server->Wait();
    }
    catch (const std::exception& e)
    {
        std::cout <<"Unhandled Exception : "<< e.what() <<'\n';

        std::cin.get();
    }


    return 0;
}


