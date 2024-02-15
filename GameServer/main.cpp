
#include "Server.h"


int main() 
{



    auto server= make_unique<psh::Server>();
    server->Start();
    server->Wait();
}
