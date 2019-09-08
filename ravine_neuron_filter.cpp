#include <fstream>
#include <string>

#include "ravine_utils.hpp"
#include "ravine_neuron_filter.hpp"

namespace RNV
{
    /* ---------------------------------------------------------------------- */
    NeuronFilter::NeuronFilter(const char* rf_file, int x, int y)
    {
        int width, height;
        if (read_rf_file(rf_file, width, height))
        {
            _win = {x, y, width, height};
        }
    }
    /* ---------------------------------------------------------------------- */
    NeuronFilter::~NeuronFilter()
    {
        delete_queue(_qin);
        delete_queue(_qout);
        if (_rf != nullptr) { delete[] _rf; }
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::open_stream()
    {
        if (has_valid_sink())
        {
            _sink->open_stream();
        }
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::start_stream()
    {
        if (!is_open())
        {
            // luanch _process_thread
            _open = true;
        }
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::stop_stream()
    {
        if (is_open())
        {
            // join _process_thread
            _open = false;
        }

    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::close_stream()
    {
        if (has_valid_sink())
        {
            _sink->close_stream();
        }
    }
    /* ---------------------------------------------------------------------- */
    void NeuronFilter::allocate_buffers(int n)
    {
        for (int k = 0; k < n; ++k)
        {
            // this should only be called from a constructor so no need
            // with wait on _qin_busy
            _qin.push(new FloatPacket(0.f0));
        }
    }
    /* ---------------------------------------------------------------------- */
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
    /* ---------------------------------------------------------------------- */
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
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::read_rf_file(const char* filepath, int& width, int& height)
    {
        bool success = false;

        std::ifstream ifs;
        ifs.open(filepath, std::ifstream::in | std::ifstream::binary);

        if (ifs.good())
        {
            std::string tmp;
            std::getline(ifs, tmp);
            if (tmp == "P5")
            {
                int white;
                ifs >> width >> height >> white;
                if (width > 0 && height > 0 && white < 256)
                {
                    _rf = new uint8_t[width*height];
                    ifs.read(_rf, width*height);
                    if (ifs) { success = true; }
                }
            }
        }

        ifs.close();
        return success;
    }
    /* ---------------------------------------------------------------------- */
    void NeuronFilter::forward_loop()
    {
        // process input when available until we receive the terminate signal
        while (persist())
        {
            // wait until the queue is available (does not imply that there is a new buffer)
            while (wait_flag(_qout_busy))
            {
                // sleeping here has little cost, so allow OS to do other things
                // (like acquire frames...)
                sleep_ms(1);
            }

            // if a new packet is available, process
            if (_qout.size() > 0)
            {
                FloatPacket* ptr = pop_queue(_qout);
                release_flag(_qout_busy);

                // process input? or forward to audio thread? or should this
                // all happen in the audio thread?


                // reuse the packet once _qin is free
                while (wait_flag(_qin_busy)) { sleep_ms(1); }
                _qin.push(ptr);
                release_flag(_qin_busy);
            }
            else
            {
                // no packets ready, release _qout
                release_flag(_qout_busy);

                // queue is empty, might as well actually wait
                sleep_ms(10);
            }

        }
    }
    /* ---------------------------------------------------------------------- */
}
