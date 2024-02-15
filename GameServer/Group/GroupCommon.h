#pragma once
#include "Group.h"

namespace psh
{
    class GroupCommon : public Group
    {
    public:
        void SetUseDB(bool use){_useDB = use;}
        
        void OnUpdate() final;
        virtual void OnEnter(SessionID id) override = 0 ;
        virtual void OnLeave(SessionID id) override = 0;
        virtual void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override = 0;
        virtual ~GroupCommon() override = 0;

    protected:

        enum ServerType : uint8
        {
            None,
            Login,
            Village,
            Easy,
            Hard,
            Pvp,
            End
        };
        
        virtual void UpdateContent(int delta) = 0;
        virtual void SendMonitor() = 0;

    private:
        bool _useDB = false;
        
        long _groupSessionCount = 0;
        long _fps = 0;
        chrono::steady_clock::time_point _nextDBSend {};
        static SessionID _monitorSession;
        static String _monitorIp;
        static Port _monitorPort;

        constexpr static uint8 _serverNo = ServerType::None;
        constexpr static GroupID _serverGroup = 1;
    };
    

}
