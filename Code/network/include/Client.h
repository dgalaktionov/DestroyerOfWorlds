#pragma once

#include "Socket.h"
#include "ConnectionManager.h"

// TODO migrate code to cpp

class Client : public AllocatorCompatible
    , public Connection::ICommunication
{
public:
    Client(const Endpoint& acRemoteEndpoint)
        : m_connection(*this, acRemoteEndpoint)
        , m_socket(acRemoteEndpoint.GetType(), false)
    {
        m_socket.Bind();
    }

    bool Send(const Endpoint& acRemoteEndpoint, Buffer aBuffer) noexcept override
    {
        Socket::Packet packet{ acRemoteEndpoint, std::move(aBuffer) };
        return m_socket.Send(packet);
    }

    uint32_t Update(const uint64_t aElapsedMilliSeconds) noexcept
    {
        uint32_t processedPackets = 0;
        m_connection.Update(aElapsedMilliSeconds);
        Outcome<Socket::Packet, Socket::Error> result = m_socket.Receive();

        while (!result.HasError())
        {
            // Route packet to a connection
            if (ProcessPacket(result.GetResult()))
                ++processedPackets;

            result = m_socket.Receive();
        }

        // TODO error handling
        return processedPackets;
    }

protected:
    bool ProcessPacket(Socket::Packet& aPacket) noexcept
    {
        if (m_connection.IsNegotiating())
        {
            if (m_connection.ProcessNegociation(&aPacket.Payload))
            {
                // OnConnected
                return true;
            }

            return false;
        }
        else if (m_connection.IsConnected())
        {
            return OnPacketReceived(Buffer::Reader(&aPacket.Payload));
        }

        return false;
    }

    bool OnPacketReceived(const Buffer::Reader &acBufferReader) noexcept
    {
        return true;
    }

private:
    Connection m_connection;
    Socket m_socket;
};