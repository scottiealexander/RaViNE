#ifndef RAVINE_FRAME_BUFFER_HPP_
#define RAVINE_FRAME_BUFFER_HPP_

#include <new>

#include "ravine_packets.hpp"

namespace RVN
{
    /* ====================================================================== */
    class FrameBuffer : public BufferPacket<uint8_t>
    {
    public:
        FrameBuffer(length_t length) :
            BufferPacket<uint8_t>(new uint8_t[length], length) {}
        virtual ~FrameBuffer();

        virtual void set_data(YUYVImagePacket& packet) = 0;
        virtual int width() const { return 0; };
        virtual int height() const { return 0; };
    };
    /* ====================================================================== */
    class FullFrameBuffer : public FrameBuffer
    {
    public:
        FullFrameBuffer(int width, int height) :
            FrameBuffer(width*height), _width(width), _height(height) {}

        void set_data(YUYVImagePacket& packet) override;

        inline int width() const override { return _width; }
        inline int height() const override { return _height; }

    private:
        int _width;
        int _height;
    };
    /* ====================================================================== */
    class CroppedFrameBuffer : public FrameBuffer
    {
    public:
        CroppedFrameBuffer(CropWindow* win) :
            FrameBuffer(win->length()), _win(win) {}

        void set_data(YUYVImagePacket& packet) override;

        inline int width() const override { return _win->width; }
        inline int height() const override { return _win->height; }

    private:
        CropWindow* _win;
    };
    /* ====================================================================== */
}
#endif
