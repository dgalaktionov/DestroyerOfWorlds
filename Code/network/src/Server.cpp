#include "Server.h"
#include "Selector.h"

Server::Server()
    : m_connectionManager(64)
    , m_v4Listener(Endpoint::kIPv4)
    , m_v6Listener(Endpoint::kIPv6)
{

}

Server::~Server()
{
}

bool Server::Start(uint16_t aPort) noexcept
{
    if (m_v4Listener.Bind(aPort) == false)
    {
        return false;
    }
    return m_v6Listener.Bind(m_v4Listener.GetPort());
}

uint32_t Server::Update(uint64_t aElapsedMilliSeconds) noexcept
{
    uint32_t processedPackets = Work();
    m_connectionManager.Update(aElapsedMilliSeconds, [this](const Endpoint & acRemoteEndpoint) { return OnClientDisconnected(acRemoteEndpoint); });
    return processedPackets;
}

uint16_t Server::GetPort() const noexcept
{
    return m_v4Listener.GetPort();
}

void Server::Disconnect(const Endpoint& acRemoteEndpoint) noexcept
{
    auto pConnection = m_connectionManager.Find(acRemoteEndpoint);
    if (pConnection && !(pConnection->GetState() == Connection::kNone))
    {
        pConnection->Disconnect();
    }
}

bool Server::Send(const Endpoint& acRemoteEndpoint, Buffer aBuffer) noexcept
{
    Socket::Packet packet{ acRemoteEndpoint, std::move(aBuffer) };

    if (acRemoteEndpoint.IsIPv6())
    {
        return m_v6Listener.Send(packet);
    }

    if (acRemoteEndpoint.IsIPv4())
    {
        return m_v4Listener.Send(packet);
    }

    return false;
}

bool Server::SendPayload(const Endpoint& acRemoteEndpoint, uint8_t *apData, size_t aLength) noexcept
{
    auto pConnection = m_connectionManager.Find(acRemoteEndpoint);
    if (!pConnection || !pConnection->IsConnected())
    {
        return false;
    }

    // FIXME memory is allocated (and freed) for every packet sent
    Buffer buffer(Socket::MaxPacketSize);
    uint32_t seq = pConnection->GetNextMessageSeq();
    Message message(seq, apData, aLength);
    Buffer::Writer writer(&buffer);
    size_t bytesWritten = 0;

    while (bytesWritten < aLength)
    {
        writer.Reset();
        pConnection->WriteHeader(writer, Connection::Header::kPayload);
        bytesWritten += message.Write(writer, bytesWritten);
        Send(acRemoteEndpoint, buffer);
    }

    return true;
}

bool Server::ProcessPacket(Socket::Packet& aPacket) noexcept
{
    Buffer::Reader reader(&aPacket.Payload);
    auto pConnection = m_connectionManager.Find(aPacket.Remote);
    if (!pConnection)
    {
        if (!m_connectionManager.IsFull())
        {
            Connection connection(*this, aPacket.Remote, true);
            m_connectionManager.Add(std::move(connection));
            pConnection = m_connectionManager.Find(aPacket.Remote);

            if (pConnection)
            {
                return !pConnection->ProcessPacket(reader).HasError();
                // TODO error handling
            }
        }

        return false;
    }
    else switch (pConnection->GetState())
    {
    case Connection::kNone:
        break;
    case Connection::kNegociating:
    {
        if (!pConnection->ProcessPacket(reader).HasError())
        {
            if (pConnection->IsConnected())
            {
                OnClientConnected(pConnection->GetRemoteEndpoint());
            }

            return true;
        }
    }
    break;

    case Connection::kConnected:
    {
        auto headerType = pConnection->ProcessPacket(reader);

        // TODO error handling
        if (!headerType.HasError() 
            && (headerType.GetResult() == Connection::Header::kPayload || headerType.GetResult() == Connection::Header::kDisconnect))
        {
            auto messageOutcome = pConnection->ReadMessage(reader);

            while (!messageOutcome.HasError())
            {
                const Message &message = messageOutcome.GetResult();

                if (message.IsComplete())
                    OnMessageReceived(aPacket.Remote, message);

                messageOutcome = pConnection->ReadMessage(reader);
            }

            return true;
        }
    }
    break;

    default:
        // TODO log about new unhandled connection state
        return false;
    }

    return false;
}

uint32_t Server::Work() noexcept
{
    uint32_t processedPackets = 0;

    Selector selector(m_v4Listener);
    while (selector.IsReady())
    {
        auto result = m_v4Listener.Receive();
        if (result.HasError())
        {
            // do some error handling
            continue;
        }
        else
        {
            // Route packet to a connection
            if (ProcessPacket(result.GetResult()))
                ++processedPackets;
        }
    }

    Selector selectorv6(m_v6Listener);
    while (selectorv6.IsReady())
    {
        auto result = m_v6Listener.Receive();
        if (result.HasError())
        {
            // do some error handling
            continue;
        }
        else
        {
            // Route packet to a connection
            if (ProcessPacket(result.GetResult()))
                ++processedPackets;
        }
    }

    return processedPackets;
}