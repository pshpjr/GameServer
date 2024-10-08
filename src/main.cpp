#define PROFILE

#include "AttackData.h"
#include "DBConnection.h"
#include "ModernObjectPool.h"
#include "Server.h"
#include "TableData.h"

// auto& manager = ProfileManager::Get();


int main()
{
    psh::SquareRange s{{-30, 0}, {30, 60}};
    //s.Rotate({1, 0}, {0, 0});

    bool result = s.Contains({15, -15});

    if (result)
    {
        std::cout << "YES" << std::endl;
    }
    else
    {
        std::cout << "NO" << std::endl;
    }


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
    catch (const std::exception &e)
    {
        std::cout << "Unhandled Exception : " << e.what() << '\n';

        std::cin.get();
    }

    return 0;
}


