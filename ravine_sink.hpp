#ifndef RAVIE_SINK_HPP_
#define RAVIE_SINK_HPP_


#include <cinttypes>

namespace ravine
{
    template <class T>
    class DataPacket
    {
    public:
        DataPacket() : _length(0) {}
        DataPacket(T* data, uint32_t length) : _data(data), _length(length) {}
        inline T* data() { return _data; }
        inline uint32_t length() { return length; }
    protected:
        T* _data = nullptr;
        uint32_t _length;
    };

    class FramePacket : public DataPacket<uint8_t>
    {
    public:
       FramePacket() : _width(0) {}
       FramePacket(uint8_t* data, uint32_t length, int width) : DataPacket(data, length), _width(width) {}
       inline int width() { return _width(); }
    private:
        int _width;
    };

    template <class T>
    class Sink
    {
    public:
        virtual bool open_stream() = 0;
        virtual bool close_stream() = 0;
        virtual bool process(T&) = 0;
    };

    /*class FileSink : public Sink<FramePacket>
     *
     * */
}
#endif
