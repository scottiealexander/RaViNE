#include "ravine_utils.hpp"
#include "ravine_neuron_filter.hpp"

namespace RNV
{
    NeuronFilter::~NeuronFilter()
    {
        delete_queue(_qin);
        delete_queue(_qout);
    }

    bool NeuronFilter::open_stream()
    {
        if (has_valid_sink())
        {
            _sink->open_stream();
        }
    }

    bool NeuronFilter::start_stream()
    {
        if (!is_open())
        {
            // luanch _process_thread
            _open = true;
        }
    }

    bool NeuronFilter::stop_stream()
    {
        if (is_open())
        {
            // join _process_thread
            _open = false;
        }

    }

    bool NeuronFilter::close_stream()
    {
        if (has_valid_sink())
        {
            _sink->close_stream();
        }
    }

    void NeuronFilter::allocate_buffers(int n)
    {
        for (int k = 0; k < n; ++k)
        {
            // this should only be called from a constructor so no need
            // with wait on _qin_busy
            _qin.push(new FloatPacket(0.f0));
        }
    }

    void NeuronFilter::filter(YUYVImagePacket* img, length_t bytes, float& act)
    {
        int32_t inc = 0;

        // in YUYV, every other element is luminance channel
        const int row_length = packet->width() * 2;

        const uint8_t* data_in = packet->data();

        const int first_col = _win->col * 2;
        const int last_col = first_col + (_win->width*2);

        const int last_row = _win->row + _win->height;

        for (int k = _win->row; k < last_row; ++k)
        {
            for (int j = first_col; j < last_col; j+=2, ++inc)
            {
                int32_t idx = k * row_length + j;
                if (idx < bytes)
                {
                    act += (float)(data_in[idx] * _rf[inc]);
                }
            }
        }
    }

    void NeuronFilter::process(YUYVImagePacket* packet, length_t bytes)
    {
        if (is_open())
        {
            // wait for use of the queue, this function needs to return asap
            // so as not to block the frame acqusition thread, so no sleep
            while (wait_flag(_qin_busy)) {/* spin until queue is available */}

            // no buffers are available, drop the frame...
            if (_qin.size() < 1) { return; }

            FloatPacket* ptr = pop_queue(_qin);
            release_flag(_qin_busy);

            // convolve image with RF
            filter(packet, bytes, ptr->get_data());

            // again, no sleep to stay quick
            while (wait_flag(_qout_busy)) {/* spin */}
            _qout.push(ptr);
            release_flag(_qout_busy);
        }
    }

}
