#pragma once

#include "Buffer.h"
#include "Outcome.h"
#include "Endpoint.h"
#include "DHChachaFilter.h"

class Socket;
class Connection
{
public:

    struct Header
    {
        enum
        {
            kNegotiation,
            kConnection,
            kCount
        };

        char Signature[2];
        uint64_t Version;
        uint64_t Type;
        uint64_t Length;
    };

    enum State
    {
        kNone,
        kNegociating,
        kConnected
    };

    enum HeaderErrors
    {
        kBadSignature,
        kBadVersion,
        kBadPacketType,
        kTooLarge,
        kUnknownChannel
    };

    struct ICommunication
    {
        virtual bool Send(const Endpoint& acRemote, Buffer aBuffer) = 0;
    };

    Connection(ICommunication& aCommunicationInterface, const Endpoint& acRemoteEndpoint, const bool acNeedsAuthentication=false);
    Connection(const Connection& acRhs) = delete;
    Connection(Connection&& aRhs) noexcept;

    ~Connection();

    Connection& operator=(Connection&& aRhs) noexcept;
    Connection& operator=(const Connection& aRhs) = delete;

    bool ProcessPacket(Buffer* apBuffer);
    bool ProcessNegociation(Buffer* apBuffer);

    bool IsNegotiating() const;
    bool IsConnected() const;

    State GetState() const;
    const Endpoint& GetRemoteEndpoint() const;

    void Update(uint64_t aElapsedMilliseconds);

protected:

    void SendNegotiation();

    Outcome<Header, HeaderErrors> ProcessHeader(Buffer::Reader& aReader);

private:

    bool WriteAuthCode(Buffer::Writer& aWriter);
    bool ReadAuthCode(Buffer::Reader& aReader, uint32_t &aCode);

    ICommunication& m_communication;
    State m_state;
    uint64_t m_timeSinceLastEvent;
    Endpoint m_remoteEndpoint;
    DHChachaFilter m_filter;
    bool m_needsAuthentication;
    uint32_t m_authCode;
};