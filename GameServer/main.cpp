#include <iostream>

#include "Server.h"



int main() 
{
    
    const auto server= make_unique<psh::Server>();
    server->SetDisableClickAndClose();
    server->Start();
    server->Wait();

    return 0;
}
