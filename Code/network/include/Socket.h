#pragma once

#include "Network.h"
#include "Outcome.h"
#include "Buffer.h"
#include "Endpoint.h"

class Socket
{
public:

    static constexpr size_t MaxPacketSize = 1200;

    enum Error
    {
        kInvalidSocket,
        kDiscardError,
        kCallFailure
    };

    struct Packet
    {
        Endpoint Remote;
        Buffer Payload;
    };

    Socket(Endpoint::Type aEndpointType = Endpoint::kIPv6, bool aBlocking = true);
    ~Socket();

    Outcome<Packet, Error> Receive();
    bool Send(const Packet& aBuffer);
    bool Bind(uint16_t aPort = 0);

    uint16_t GetPort() const;

protected:

    bool Bindv6(uint16_t aPort);
    bool Bindv4(uint16_t aPort);

private:

    friend class Selector;

    Socket_t m_sock;
    uint16_t m_port;
    Endpoint::Type m_type;
};