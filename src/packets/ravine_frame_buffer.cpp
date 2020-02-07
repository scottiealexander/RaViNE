
#include "ravine_utils.hpp"
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
    void FullFrameBuffer::set_data(YUYVImagePacket* packet, length_t bytes)
    {
        const int32_t end = RVN_MIN(this->length()*2, bytes);

        // copy from input packet to internal storage
        const uint8_t* data_in = packet->data();
        uint8_t* data = this->data();

        int32_t inc = 0;

        // +=2 for YUYV encoding
        for (int32_t k = 0; k < end; k += 2, ++inc)
        {
            data[inc] = data_in[k];
        }
    }
    /* ====================================================================== */
    void CroppedFrameBuffer::set_data(YUYVImagePacket* packet, length_t bytes)
    {
        int32_t inc = 0;

        // in YUYV, every other element is luminance channel
        const int row_length = packet->width() * 2;

        const uint8_t* data_in = packet->data();
        uint8_t* data = this->data();

        int first_col = _win->col * 2;
        int last_col = first_col + (_win->width*2);

        int last_row = _win->row + _win->height;

        for (int k = _win->row; k < last_row; ++k)
        {
            for (int j = first_col; j < last_col; j+=2, ++inc)
            {
                int32_t idx = k * row_length + j;
                if (idx < bytes)
                {
                    data[inc] = data_in[idx];
                }
            }
        }
    }
    /* ====================================================================== */
}

