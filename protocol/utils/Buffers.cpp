#include "Buffers.hpp"
#include <cstring>

WritableBuffer::WritableBuffer(uint8_t* buffer, size_t len)
    : m_write_ptr(buffer)
    , m_write_pos(0)
    , m_write_size(len)
{
}
WritableBuffer::~WritableBuffer() {}

bool WritableBuffer::write(const void* src, size_t len)
{
    if (len > m_write_size - m_write_pos)
        return false;
    memcpy(m_write_ptr + m_write_pos, src, len);
    m_write_pos += len;
    return true;
}

bool WritableBuffer::seek_write(size_t pos)
{
    if (pos > m_write_size)
        return false;
    m_write_pos = pos;
    return true;
}

uint8_t* WritableBuffer::get_write_ptr() const
{
    return m_write_ptr;
}

size_t WritableBuffer::get_write_pos() const
{
    return m_write_pos;
}

size_t WritableBuffer::get_write_size() const
{
    return m_write_size;
}

void WritableBuffer::reset()
{
    memset(m_write_ptr, 0, m_write_size);
}


ReadableBuffer::ReadableBuffer(const uint8_t* buffer, size_t len)
    : m_read_ptr(buffer)
    , m_read_pos(0)
    , m_read_size(len)
{
}
ReadableBuffer::~ReadableBuffer() {}

bool ReadableBuffer::read(void* dst, size_t len)
{
    if (len > m_read_size - m_read_pos)
        return false;
    memcpy(dst, m_read_ptr + m_read_pos, len);
    m_read_pos += len;
    return true;
}

bool ReadableBuffer::write_to(WritableBuffer& stream, size_t len)
{
    size_t space = stream.get_write_size() - stream.get_write_pos();
    if(len == 0)
        len = m_read_size;

    if (len > space)
        return false;

    return stream.write(m_read_ptr, len);
}

bool ReadableBuffer::seek_read(size_t pos)
{
    if (pos > m_read_size)
        return false;
    m_read_pos = pos;
    return true;
}

const uint8_t* ReadableBuffer::get_read_ptr() const
{
    return m_read_ptr;
}

size_t ReadableBuffer::get_read_pos() const
{
    return m_read_pos;
}

size_t ReadableBuffer::get_read_size() const
{
    return m_read_size;
}

ByteBuffer::ByteBuffer(uint8_t* buffer, size_t len)
    : ReadableBuffer(buffer, len)
    , WritableBuffer(buffer, len)
{
}

ByteBuffer::~ByteBuffer()
{
}
