#ifndef RAVINE_PACKETS_HPP_
#define RAVINE_PACKETS_HPP_

#include <cinttypes>
#include "ravine_packet_base.hpp"

#define RVN_MIN(x, y) ((x) < (y) ? (x) : (y))
#define RVN_MAX(x, y) ((x) > (y) ? (x) : (y))

namespace RVN
{
    typedef int32_t length_t;
    /* ====================================================================== */
    struct CropWindow
    {
        inline length_t length() const
        {
            return ((length_t)width) * ((length_t)height);
        }
        int col;
        int row;
        int width;
        int height;
    };
    /* ====================================================================== */
    template <class T>
    class BufferPacket : public Packet<T*>
    {
    public:
        BufferPacket(T* data, length_t length) : Packet<T*>(data), _length(length) {}
        BufferPacket() : Packet<T*>(nullptr), _length(0) {}
        inline length_t length() const { return _length; }
        inline bool is_empty() const { return this->_data == nullptr; }
    protected:
        length_t _length;
    };
    /* ====================================================================== */
    template <class T>
    class FramePacket : public BufferPacket<T>
    {
    public:
        FramePacket(T* data, length_t length, int width) : BufferPacket<T>(data, length), _width(width) {}
        FramePacket() : _width(0) {}
        inline int width() const { return _width; }
    protected:
        int _width;
    };
    /* ====================================================================== */
    typedef FramePacket<uint8_t> YUYVImagePacket;
    /* ====================================================================== */
}

#endif
