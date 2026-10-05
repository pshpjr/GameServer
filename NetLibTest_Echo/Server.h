#pragma once

#include <cstdint>

#include "IOCP.h"

namespace psh
{
    class Server final : public IOCP
    {
    public:
        explicit Server(std::uint16_t port = 6000, bool trace = false);

        void OnConnect(SessionID sessionId, const SockAddr& info) override;
        void OnDisconnect(SessionID sessionId, int error) override;
        void OnRecvPacket(SessionID sessionId, RecvBuffer& buffer) override;
        void OnMonitorRun() override;

    private:
        void Trace(const char* event, SessionID sessionId, std::uint64_t value = 0, int error = 0) const;

        bool _trace;
    };
}
