
#include "ravine_frame_buffer.hpp"

namespace RVN
{
    /* ====================================================================== */
    FrameBuffer::~FrameBuffer()
    {
        if (_data != nullptr)
        {
            delete[] _data;
        }
    }
    /* ====================================================================== */
    void FullFrameBuffer::set_data(YUYVImagePacket& packet)
    {
        int32_t inc = 0;

        int32_t end = RVN_MIN(_length*2, this->length());

        // copy from input packet to internal storage
        uint8_t* data_in = packet.data();
        uint8_t* data = this->data();

        // +=2 for YUYV encoding
        for (int32_t k = 0; k < end; k += 2, ++inc)
        {
            data[inc] = data_in[k];
        }
    }
    /* ====================================================================== */
    void CroppedFrameBuffer::set_data(YUYVImagePacket& packet)
    {
        int32_t inc = 0;

        // in YUYV, every other element is luminance channel
        int row_length = packet.width() * 2;

        uint8_t* data_in = packet.data();
        uint8_t* data = this->data();

        for (int k = _win->row; k < _win->height; ++k)
        {
            for (int j = (_win->col * 2); j < (_win->width*2); j+=2, ++inc)
            {
                int32_t idx = (k * row_length + j);
                if (idx < this->length())
                {
                    data[inc] = data_in[idx];
                }
            }
        }
    }
    /* ====================================================================== */
}

