#include <new>

#include "ravine_buffer.hpp"

namespace ravine
{
    /* ====================================================================== */
    FrameBuffer::FrameBuffer(uint32_t length) : _length(length)
    {
        _data = new uint8_t[_length];
    }
    /* ---------------------------------------------------------------------- */
    FrameBuffer::~FrameBuffer()
    {
        if (_data != nullptr)
        {
            delete[] _data;
        }
    }
    /* ====================================================================== */
    void FullFrameBuffer::set_data(void* data, uint32_t size, int width) override
    {
        uint32_t inc = 0;

        uint32_t end = min(_length*2, size);

        // +=2 for YUYV encoding
        for (uint32_t k = 0; k < end; k += 2, ++inc)
        {
            _data[inc] = data[k];
        }
    }
    /* ====================================================================== */
    void CroppedFrameBuffer::set_data(void* data, uint32_t size, int width)
    {
        int inc = 0;

        // in YUYV, every other element is luminance channel
        int row_length = width * 2;

        for (int k = _win->row; k < _win->height; ++k, ++inc)
        {
            for (int j = (_win->col * 2); j < (_win->width*2); j+=2)
            {
                int idx = (k * row_length + j);
                if (idx < size)
                {
                    _data[inc] = data[idx];
                }
            }
        }
    }
    /* ====================================================================== */
}

