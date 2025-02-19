#pragma once
#include <cstddef>
#include <cstdint>

class WritableBuffer
{
private:
    uint8_t* m_write_ptr;
    size_t m_write_pos;
    size_t m_write_size;

public:
    WritableBuffer(uint8_t* buffer, size_t len);
    virtual ~WritableBuffer();

    bool write(const void* src, size_t len);
    bool pad(char c, size_t len);
    bool seek_write(size_t pos);
    uint8_t* get_write_ptr() const;
    size_t get_write_pos() const;
    size_t get_write_size() const;
    void reset();
};

class ReadableBuffer
{
private:
    const uint8_t* m_read_ptr;
    size_t m_read_pos;
    size_t m_read_size;

public:
    ReadableBuffer(const uint8_t* buffer, size_t len);
    virtual ~ReadableBuffer();

    bool read(void* dst, size_t len);
    bool write_to(WritableBuffer& stream, size_t len = 0);
    bool seek_read(size_t pos);
    const uint8_t* get_read_ptr() const;
    size_t get_read_pos() const;
    size_t get_read_size() const;
};

class ByteBuffer : public ReadableBuffer, public WritableBuffer
{
public:
    ByteBuffer(uint8_t* buffer, size_t len);
    virtual ~ByteBuffer();
};

template<size_t Size>
class AllocatedByteBuffer : public ByteBuffer
{
private:
    uint8_t m_buffer[Size];

public:
    AllocatedByteBuffer() : ByteBuffer(m_buffer, Size) {}
    virtual ~AllocatedByteBuffer() {}
};
