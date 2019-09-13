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
        virtual ~BufferPacket() {}
        virtual inline length_t length() const { return _length; }
        inline bool isvalid() const { return this->_data != nullptr; }
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
    template <class T>
    class ScalarPacket : public Packet<T>
    {
    public:
        inline T& get_data() { return this->_data; }
        inline void set_data(T x) { this->_data = x; }
    };
    /* ====================================================================== */
    typedef ScalarPacket<float> FloatPacket;
    /* ====================================================================== */
    class EventPacket : public Packet<uint8_t>
    {
    public:
        EventPacket() : Packet<uint8_t>(0x00), _time(-1.0f) {}
        EventPacket(uint8_t d, float time) : Packet<uint8_t>(d), _time(time) {}

        inline void operator=(const EventPacket* other)
        {
            this->_data = other->data();
            _time = other->timestamp();
        }
        inline float timestamp() const { return _time; }
    protected:
        float _time;
    };
    /* ====================================================================== */
    class AudioPacket : public BufferPacket<float>
    {
    public:
        AudioPacket(float* data, length_t length, float time) :
            BufferPacket<float>(data, length), _time(time) {}
        virtual ~AudioPacket() {}
        inline float timestamp() const { return _time; }
    protected:
        float _time;
    };
    /* ====================================================================== */
}

#endif
