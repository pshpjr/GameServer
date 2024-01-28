#include "stdafx.h"
#include "Server.h"

int main() 
{

	auto server = make_unique<Server>();
	server->Start();
	server->Wait();


	return 0;
}