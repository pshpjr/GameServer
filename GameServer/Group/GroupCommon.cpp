#include "GroupCommon.h"

#include "CLogger.h"
#include "CoreGlobal.h"
#include "IOCP.h"
#include "SendBuffer.h"

void psh::GroupCommon::OnUpdate()
{
    UpdateContent(1);
    _fps++;

    
    if ( std::chrono::steady_clock::now() < _nextDBSend )
    {
        return ;
    }


    if(!_useDB)
    {
    }
    
    // auto& iocp = *_iocp;
    // if ( iocp.IsValidSession(_monitorSession) )
    // {
    //     SendMonitor();
    // }
    // else
    // {
    //     auto clientResult = iocp.GetClientSession(_monitorIp, _monitorPort);
    //     
    //     if (  clientResult.HasValue() )
    //     {
    //         gLogger->Write(L"System", LogLevel::Error, L"Monitor server Connect Success\n");
    //         _monitorSession = clientResult.Value();
    //
    //         auto loginBuffer = SendBuffer::Alloc();
    //         loginBuffer << en_PACKET_SS_MONITOR_LOGIN << _serverGroup << _serverNo;
    //         SendPacket(_monitorSession, loginBuffer);
    //     }
    //
    // }


    _nextDBSend += 1s;
    _fps = 0;
}

psh::GroupCommon::~GroupCommon()
{
}
