#include <iostream>

#include "Server.h"
#include "TableData.h"
#include "AttackData.h"
#include "DBConnection.h"
int main()
{
    DBConnection::LibInit();
    SEND_OFFSETS::Init(); 
    psh::ATTACK::Init();
    const auto server = make_unique<psh::Server>();
    server->SetDisableClickAndClose();
    server->Start();
    server->Wait();

    return 0;
}
