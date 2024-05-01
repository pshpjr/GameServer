#include "MonitorClient.h"
#include "IOCP.h"
#include "SendBuffer.h"
#include "MonitorProtocol.h"

void MonitorClient::Connect()
{
    if (_monitorSession == InvalidSessionID())
    {
        auto client = _server->GetClientSession(_ip,_port);
        if (client.HasError())
        {
            return;
        }
        _monitorSession = client.Value();
        SendLogin();
    }
}

void MonitorClient::SendLogin()
{
    auto buffer = SendBuffer::Alloc();

    buffer << en_PACKET_SS_MONITOR_LOGIN << WORD(1) << _serverType;
    _server->SendPacket(_monitorSession, buffer);
}

void MonitorClient::SendMonitorData(char type, int value)
{
    Connect();

    auto buffer = SendBuffer::Alloc();

    buffer << en_PACKET_SS_MONITOR_DATA_UPDATE << WORD(1) << _serverType << type << value << static_cast<int>(time(nullptr));
    if (_server->SendPacket(_monitorSession, buffer) == false) 
    {
        _monitorSession = InvalidSessionID();
    }
}