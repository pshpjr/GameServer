#include "Server.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <syncstream>
#include <thread>

#include "RecvBuffer.h"
#include "SendBuffer.h"

namespace psh
{
    Server::Server(std::uint16_t port, bool trace) : IOCP(), _trace(trace)
    {
        // A zero static key selects the unencrypted, two-byte LAN header.
        if (!Init(L"0.0.0.0", port, 128, 4, 4, 0))
        {
            throw std::runtime_error("Failed to initialize the echo server.");
        }

        SetDefaultTimeout(0);
    }

    void Server::Trace(const char* event, SessionID sessionId, std::uint64_t value, int error) const
    {
        if (!_trace)
        {
            return;
        }

        const auto time = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::osyncstream(std::clog)
            << "[EchoTrace] us=" << time << " thread=" << std::this_thread::get_id()
            << " event=" << event << " session=" << sessionId.id << " index=" << sessionId.index
            << " value=" << value << " error=" << error << std::endl;
    }

    void Server::OnConnect(SessionID sessionId, const SockAddr_in&)
    {
        Trace("connect", sessionId);
    }

    void Server::OnDisconnect(SessionID sessionId, int error)
    {
        Trace("disconnect", sessionId, 0, error);
    }

    void Server::OnMonitorRun()
    {
        PrintMonitorString();
    }

    void Server::OnRecvPacket(SessionID sessionId, RecvBuffer& buffer)
    {
        if (buffer.CanPopSize() != sizeof(std::uint64_t))
        {
            Trace("invalid_length", sessionId, buffer.CanPopSize());
            // NetLib requires the entire payload to be consumed by this callback.
            while (buffer.CanPopSize() > 0)
            {
                char discarded{};
                buffer >> discarded;
            }
            DisconnectSession(sessionId);
            return;
        }

        std::uint64_t value{};
        buffer >> value;
        Trace("recv", sessionId, value);

        auto response = SendBuffer::Alloc();
        response << value;
        const bool queued = SendPacket(sessionId, response);
        // This records the API result, not socket send completion.
        Trace(queued ? "send_api_true" : "send_api_false", sessionId, value);
    }
}
