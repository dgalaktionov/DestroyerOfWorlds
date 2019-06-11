#include "Message.h"

#include <algorithm>


Message& Message::Merge(Message& aLhs, Message& aRhs) noexcept
{
    if (aLhs.GetFistValidOffset() > aRhs.GetFistValidOffset())
    {
        std::swap(aLhs, aRhs);
    }

    auto itLhs = aLhs.m_slices.begin();

    for (auto itRhs = aRhs.m_slices.cbegin(); itRhs != aRhs.m_slices.cend(); itRhs++)
    {
        if (!itRhs->m_empty && itRhs->m_len > 0)
        {
            // find the first empty space in aLhs that may contain this slice
            while (itLhs->GetEndOffset() < itRhs->GetEndOffset() || itLhs->m_offset > itRhs->m_offset)
                itLhs++;

            // if for some reason there is an overlap error, we cannot merge these two messages
            if (itLhs == aLhs.m_slices.end())
                break;

            if (itRhs->m_offset > itLhs->m_offset)
            {
                // add empty slice before
                aLhs.m_slices.insert(itLhs, std::move(Slice(itLhs->m_offset, itRhs->m_offset - itLhs->m_offset)));
            }

            size_t endOffset = itRhs->GetEndOffset();

            // insert our new slice
            aLhs.m_slices.insert(itLhs, std::move(*itRhs));

            if (endOffset < itLhs->GetEndOffset())
            {
                // adjust current empty space after the newly inserted element
                itLhs->m_len = itLhs->GetEndOffset() - endOffset;
                itLhs->m_offset = endOffset;
            }
            else
            {
                // remove current empty space
                itLhs = aLhs.m_slices.erase(itLhs);
            }
        }
    }

    // Consolidation phase: we want to merge any two contiguous data slices
    auto it = aLhs.m_slices.begin();

    while (it != aLhs.m_slices.end())
    {
        auto next = std::next(it, 1);

        if (next != aLhs.m_slices.end() && !it->m_empty && !next->m_empty)
        {
            Buffer buffer(it->m_len + next->m_len);

            std::copy(it->m_data.GetData(), it->m_data.GetData() + it->m_len, buffer.GetWriteData());
            std::copy(next->m_data.GetData(), next->m_data.GetData() + next->m_len, buffer.GetWriteData() + it->m_len);

            next->m_offset = it->m_offset;
            next->m_len = it->m_len + next->m_len;
            next->m_data = std::move(buffer);

            it = aLhs.m_slices.erase(it);
        }
        else
        {
            it++;
        }

    }

    // The merged message is not valid anymore
    aRhs.m_len = 0;

    return aLhs;
}

Message::Message(uint32_t aSeq, uint8_t *apData, size_t aLen) noexcept
    : m_slices({std::move(Message::Slice(apData, aLen))})
    , m_len(aLen)
    , m_seq(aSeq)
{

}

Message::Message(Buffer::Reader& aReader) noexcept
    : m_slices()
    , m_len(0)
    , m_seq(0)
{
    aReader.ReadBytes((uint8_t *)&m_seq, sizeof(m_seq));

    if (aReader.ReadBits(m_len, Message::MessageLenBits) && m_len <= Message::MaxMessageSize)
    {
        m_slices.push_front(Slice(aReader, m_len));
        
        Slice& slice = m_slices.front();
        
        if (slice.m_empty)
        {
            // Could not read slice for some reason. This message should be invalid
            m_len = 0;
        }
        else
        {
            if (slice.m_offset > 0)
            {
                // add empty slice before
                m_slices.push_front(Slice((size_t) 0, slice.m_offset));
            }

            if (slice.GetEndOffset() < m_len)
            {
                // add empty slice after
                m_slices.push_back(Slice(slice.GetEndOffset(), m_len - slice.GetEndOffset()));
            }
        }
    }
}

Message::Message(Message && aRhs) noexcept
{
    this->operator=(std::move(aRhs));
}

Message::Message(const Message & acRhs) noexcept
{
    this->operator=(acRhs);
}

Message::~Message() noexcept
{
    
}

Message & Message::operator=(Message && aRhs) noexcept
{
    this->m_seq = aRhs.m_seq;
    this->m_len = aRhs.m_len;
    this->m_slices = std::move(aRhs.m_slices);

    aRhs.m_seq = 0;
    aRhs.m_len = 0;

    return *this;
}

Message & Message::operator=(const Message & acRhs) noexcept
{
    this->m_seq = acRhs.m_seq;
    this->m_len = acRhs.m_len;
    this->m_slices = acRhs.m_slices;

    return *this;
}

Message & Message::operator+(Message& aRhs) noexcept
{
    return Message::Merge(*this, aRhs);
}

uint32_t Message::GetSeq() const noexcept
{
    return m_seq;
}

size_t Message::GetLen() const noexcept
{
    return m_len;
}

// Returns true if this message is ready (has only one slice)
bool Message::IsComplete() const noexcept
{
    return IsValid() && m_slices.size() == 1;
}

bool Message::IsValid() const noexcept
{
    return m_len > 0 && m_slices.size() > 0;
}

// Do not call this if IsComplete() returns false or face undefined behavior
Buffer::Reader Message::GetData() const noexcept
{
    return Buffer::Reader((Buffer *)&m_slices.front().m_data);
}

// Returns the number of bytes of real data (not headers) written
size_t Message::Write(Buffer::Writer& aWriter, size_t aOffset) const noexcept
{
    if (!IsComplete())
        return 0;   // this should never happen(tm)

    size_t availableBytes = aWriter.GetSize() - aWriter.GetBytePosition();

    if (availableBytes <= Message::HeaderBytes)
    {
        // Don't even try to write a slice because it won't fit
        return 0;
    }


    aWriter.WriteBytes((uint8_t *)&m_seq, sizeof(m_seq));
    aWriter.WriteBits(m_len, Message::MessageLenBits);
    aWriter.WriteBits(aOffset, Message::MessageLenBits);

    availableBytes = aWriter.GetSize() - aWriter.GetBytePosition();
    size_t bytesToWrite = std::min(availableBytes, m_len - aOffset);
    aWriter.WriteBytes(m_slices.front().m_data.GetData()+aOffset, bytesToWrite);
    
    return bytesToWrite;
}

size_t Message::GetFistValidOffset() const noexcept
{
    size_t offset = 0;

    for (auto it = m_slices.cbegin(); it != m_slices.cend(); it++)
    {
        if (!it->m_empty)
        {
            offset = it->m_offset;
            break;
        }
    }

    return offset;
}

Message::Slice::Slice(size_t aOffset, size_t aLen) noexcept
    : m_offset(aOffset)
    , m_len(aLen)
    , m_empty(true)
{

}

Message::Slice::Slice(uint8_t *apData, size_t aLen) noexcept
    : m_offset(0)
    , m_len(aLen)
    , m_empty(false)
{
    m_data = Buffer(aLen);
    std::copy(apData, apData + aLen, m_data.GetWriteData());
}

Message::Slice::Slice(Buffer::Reader& aReader, size_t aMessageLength) noexcept
    : m_offset(0)
    , m_len(0)
    , m_empty(true)
{
    if (aReader.ReadBits(m_offset, Message::MessageLenBits) && m_offset < aMessageLength)
    {
        m_len = std::min(aReader.GetSize() - aReader.GetBytePosition(), aMessageLength - m_offset);
        m_data = Buffer(m_len);
        m_empty = !aReader.ReadBytes(m_data.GetWriteData(), m_len);
    }
}

Message::Slice::Slice(Slice && aRhs) noexcept
{
    this->operator=(std::move(aRhs));
}

Message::Slice::Slice(const Slice & acRhs) noexcept
{
    this->operator=(acRhs);
}

Message::Slice& Message::Slice::operator=(Slice && aRhs) noexcept
{
    this->m_offset = aRhs.m_offset;
    this->m_len = aRhs.m_len;
    this->m_empty = aRhs.m_empty;
    this->m_data = std::move(aRhs.m_data);

    aRhs.m_offset = 0;
    aRhs.m_len = 0;
    aRhs.m_empty = true;

    return *this;
}

Message::Slice& Message::Slice::operator=(const Slice & acRhs) noexcept
{
    this->m_offset = acRhs.m_offset;
    this->m_len = acRhs.m_len;
    this->m_empty = acRhs.m_empty;
    this->m_data = acRhs.m_data;

    return *this;
}

size_t Message::Slice::GetEndOffset() const noexcept
{
    return m_offset+m_len;
}
