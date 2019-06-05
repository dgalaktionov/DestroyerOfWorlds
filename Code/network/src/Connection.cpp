#include "Connection.h"
#include "StackAllocator.h"



struct NullCommunicationInterface : public Connection::ICommunication
{
    bool Send(const Endpoint& acRemote, Buffer aBuffer) override
    {
        (void)acRemote;
        (void)aBuffer;

        return false;
    }
};

static NullCommunicationInterface s_dummyInterface;

static const char* s_headerSignature = "MG";

Connection::Connection(ICommunication& aCommunicationInterface, const Endpoint& acRemoteEndpoint, const bool acNeedsAuthentication)
    : m_communication{ aCommunicationInterface }
    , m_state{kNegociating}
    , m_timeSinceLastEvent{0}
    , m_remoteEndpoint{acRemoteEndpoint}
    , m_needsAuthentication{acNeedsAuthentication}
{
    if (acNeedsAuthentication)
    {
        // TODO secure random
        m_authCode = 24;
    }
    else
    {
        m_authCode = 0;
    }

}

Connection::Connection(Connection&& aRhs) noexcept
    : m_communication{aRhs.m_communication}
    , m_state{std::move(aRhs.m_state)}
    , m_timeSinceLastEvent{std::move(aRhs.m_timeSinceLastEvent)}
    , m_remoteEndpoint{std::move(aRhs.m_remoteEndpoint)}
    , m_needsAuthentication{aRhs.m_needsAuthentication}
    , m_authCode{aRhs.m_authCode}
{
    aRhs.m_communication = s_dummyInterface;
    aRhs.m_state = kNone;
    aRhs.m_timeSinceLastEvent = 0;
    aRhs.m_authCode = 0;
}

Connection::~Connection()
{
}

Connection& Connection::operator=(Connection&& aRhs) noexcept
{
    m_communication = aRhs.m_communication;
    m_state = aRhs.m_state;
    m_timeSinceLastEvent = aRhs.m_timeSinceLastEvent;
    m_remoteEndpoint = std::move(aRhs.m_remoteEndpoint);
    m_needsAuthentication = aRhs.m_needsAuthentication;
    m_authCode = aRhs.m_authCode;

    aRhs.m_communication = s_dummyInterface;
    aRhs.m_state = kNone;
    aRhs.m_timeSinceLastEvent = 0;
    aRhs.m_authCode = 0;

    return *this;
}

bool Connection::ProcessPacket(Buffer* apBuffer)
{
    Buffer::Reader reader(apBuffer);
    
    auto header = ProcessHeader(reader);
    if (header.HasError())
    {
        return false;
    }

    if (header.GetResult().Type == Header::kNegotiation)
    {
        /*
        Even if we consider ourselves connected, the other party (probably the server) may be still waiting
         for our confirmation...
        */
        return ProcessNegociation(apBuffer);
    }
    else
    {
        m_timeSinceLastEvent = 0;
    }

    return true;
}

bool Connection::ProcessNegociation(Buffer* apBuffer)
{
    Buffer::Reader reader(apBuffer);

    auto header = ProcessHeader(reader);
    if (header.HasError() || header.GetResult().Type != Header::kNegotiation)
        return false;

    if (!m_filter.ReceiveConnect(&reader))
    {
        // Drop connection if key is not accepted
        m_state = Connection::kNone;
        return false;
    }

    if (m_needsAuthentication)
    {
        // We are a server that needs to challenge clients
        uint32_t otherCode = 0;

        if (header.GetResult().Length >= sizeof(m_authCode) && ReadAuthCode(reader, otherCode))
        {
            if (otherCode == m_authCode)
            {
                // We got a correct challenge code back
                m_state = kConnected;
                return true;
            }
            else
            {
                // Wrong challenge code, drop connection
                m_state = Connection::kNone;
                return false;
            }
        }
        else
        {
            // No challenge code received yet
            return false;
        }
    }
    else if (header.GetResult().Length >= sizeof(m_authCode) && ReadAuthCode(reader, m_authCode))
    {
        // We (client) assume to be connected and send back the challenge code
        m_state = kConnected;
        SendNegotiation();
    }

    return IsNegotiating() || IsConnected();
}

bool Connection::IsNegotiating() const
{
    return m_state == kNegociating;
}

bool Connection::IsConnected() const
{
    return m_state == kConnected;
}

Connection::State Connection::GetState() const
{
    return m_state;
}

const Endpoint& Connection::GetRemoteEndpoint() const
{
    return m_remoteEndpoint;
}

void Connection::Update(uint64_t aElapsedMilliseconds)
{
    m_timeSinceLastEvent += aElapsedMilliseconds;

    // Connection is considered timed out if no data is received in 15s (TODO: make this configurable)
    if (m_timeSinceLastEvent > 15 * 1000)
    {
        m_state = kNone;
        return;
    }

    switch (m_state)
    {
    case Connection::kNone:
        break;
    case Connection::kNegociating:
        SendNegotiation();
        break;
    case Connection::kConnected:
        break;
    default:
        break;
    }
}

void Connection::SendNegotiation()
{
    Header header;
    header.Signature[0] = s_headerSignature[0];
    header.Signature[1] = s_headerSignature[1];
    header.Version = 1;
    header.Type = Header::kNegotiation;
    header.Length = 0;

    if (m_authCode > 0)
        header.Length += sizeof(m_authCode);

    StackAllocator<1 << 13> allocator;
    auto* pBuffer = allocator.New<Buffer>(1200);

    Buffer::Writer writer(pBuffer);
    writer.WriteBytes((const uint8_t*)header.Signature, 2);
    writer.WriteBits(header.Version, 6);
    writer.WriteBits(header.Type, 3);
    writer.WriteBits(header.Length, 11);

    m_filter.PreConnect(&writer);

    if (m_authCode > 0)
        WriteAuthCode(writer);

    m_communication.Send(m_remoteEndpoint, *pBuffer);

    allocator.Delete(pBuffer);
}

Outcome<Connection::Header, Connection::HeaderErrors> Connection::ProcessHeader(Buffer::Reader& aReader)
{
    Header header;

    aReader.ReadBytes((uint8_t*)&header.Signature, 2);

    if (header.Signature[0] != 'M' || header.Signature[1] != 'G')
        return kBadSignature;

    aReader.ReadBits(header.Version, 6);

    if (header.Version != 1)
        return kBadVersion;

    aReader.ReadBits(header.Type, 3);

    if (header.Type >= Header::kCount)
        return kBadPacketType;

    aReader.ReadBits(header.Length, 11);

    if (header.Length > 1200)
        return kTooLarge;

    return header;
}

bool Connection::WriteAuthCode(Buffer::Writer &aWriter)
{
    return aWriter.WriteBytes((uint8_t *) &m_authCode, sizeof(m_authCode));
}

bool Connection::ReadAuthCode(Buffer::Reader &aReader, uint32_t &aCode)
{
    return aReader.ReadBytes((uint8_t *) &aCode, sizeof(m_authCode));
}
