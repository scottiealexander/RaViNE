#ifndef RAVINE_BUFFER_PACKET_HPP_
#define RAVINE_BUFFER_PACKET_HPP_

#include <cinttypes>
#include "ravine_packet_base.hpp"

namespace RVN
{
    typedef int32_t length_t;

    template <class T>
    class BufferPacket : public Packet<T*>
    {
    public:
        BufferPacket(T* data, length_t length) : Packet<T*>(data), _length(length) {}
        BufferPacket() : _length(0), _data(nullptr) {}
        inline length_t length() { return _length; }
        inline bool is_empty() { return _data == nullptr; }
    protected:
        length_t _length;
    };

    template <class T>
    class FramePacket : public BufferPacket<T>
    {
    public:
        FramePacket(T* data, length_t length, int width) : BufferPacket<T>(data, length), _width(width) {}
        FramePacket() : _width(0) {}
        inline int width() { return _width; }
    protected:
        int _width;
    };

    typedef FramePacket<uint8_t> YUYVImagePacket;
}

#endif
