#pragma once
#include <Types.h>
#include <Session.h>
class MonitorClient
{
public:
	MonitorClient(IOCP* server, String ip, Port port,char  serverType) : _server(server),_ip(ip),_port(port),_serverType(serverType)
	{

	}

	void SendMonitorData(char type, int value);

private:
	void Connect();
	void UseMonitor() { _useMonitor = true; };
	void UnuseMonitor() { _useMonitor = false; };
	void SendLogin();

	bool _useMonitor = true;
	IOCP* _server = nullptr;
	char _serverType = 0;
    SessionID _monitorSession = InvalidSessionID();
	String _ip = {};
	Port _port = {};
};

