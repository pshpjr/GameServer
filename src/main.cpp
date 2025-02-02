﻿#define PROFILE

#include "AttackData.h"
#include "DBConnection.h"
#include "Server.h"
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


