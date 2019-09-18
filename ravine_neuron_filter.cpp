#include <fstream>
#include <string>
#include <cstdio>
#include <cmath>

#include "ravine_utils.hpp"
#include "ravine_neuron_filter.hpp"

namespace RVN
{
    /* ---------------------------------------------------------------------- */
    float mean(const uint8_t* data, int length)
    {
        float mn = 0.0f;
        for (int k = 0; k < length; ++k)
        {
            mn += data[k];
        }
        return mn / length;
    }
    /* ---------------------------------------------------------------------- */
    float two_norm(const uint8_t* data, int length, float& mn)
    {
        mn = mean(data, length);
        float mag = 0.0f;
        for (int k = 0; k < length; ++k)
        {
            float tmp = ((float)data[k]) - mn;
            mag += tmp*tmp;
        }

        return mag;
    }
    /* ---------------------------------------------------------------------- */
    NeuronFilter::NeuronFilter(const char* rf_file, int x, int y, int nbuf) :
        _open(false), _isvalid(true), _rf_mag(0.0f), _threshold(0.0f)
    {
        int width, height;
        if (read_rf_file(rf_file, width, height))
        {
            printf("[NEURON]: %d x %d @ (%d, %d)\n", width, height, x, y);
            _win = {x, y, width, height};

            // we're not yet parallel, so no need to check busy flags
            allocate_buffers(nbuf);

            printf("[NEURON]: %d buffers allocated\n", _qin.size());
        }
        else
        {
            set_error_msg("Failed to read rf file");
        }
    }
    /* ---------------------------------------------------------------------- */
    NeuronFilter::~NeuronFilter()
    {
        delete_queue(_qin);
        delete_queue(_qout);
        if (_rf != nullptr)
        {
            delete[] _rf;
        }
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::open_stream()
    {
        open_sink_stream();
        return start_stream();
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::start_stream()
    {
        if (!is_open() && isvalid())
        {
            (void)persist();

            _process_thread = std::thread(&NeuronFilter::forward_loop, this);
            _open = true;
        }
        else
        {
            printf("[NEURON]: failed starting stream\n");
        }
        return isvalid() && is_open();
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::stop_stream()
    {
        if (is_open())
        {
            _state_continue.clear();
            _open = false;
        }
        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool NeuronFilter::close_stream()
    {
        if (is_open()) { (void)stop_stream(); }
        _process_thread.join();

        close_sink_stream();

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    void NeuronFilter::allocate_buffers(int n)
    {
        for (int k = 0; k < n; ++k)
        {
            // this should only be called from a constructor so no need
            // with wait on _qin_busy
            _qin.push(new FloatPacket());
        }
    }
    /* ---------------------------------------------------------------------- */
    void NeuronFilter::filter(YUYVImagePacket* packet, length_t bytes, float& act)
    {
        act = 0.0f;
        int32_t inc = 0;

        float frame_mag = 0.0f;
        float xy = 0.0f;
        float yi;

        // in YUYV, every other element is luminance channel
        const int row_length = packet->width() * 2;

        const uint8_t* data_in = packet->data();

        float frame_mean = mean(data_in, bytes);

        const int first_col = _win.col * 2;
        const int last_col = first_col + (_win.width*2);

        const int last_row = _win.row + _win.height;

        for (int k = _win.row; k < last_row; ++k)
        {
            for (int j = first_col; j < last_col; j+=2, ++inc)
            {
                int32_t idx = k * row_length + j;
                if (idx < bytes)
                {
                    yi = ((float)data_in[idx]) - frame_mean;
                    frame_mag += yi*yi;
                    xy += yi * (((float)_rf[inc]) - _rf_mean);
                }
            }
        }

        const float mx = RVN_MAX(_rf_mag, frame_mag);
        act = xy / mx / sqrt(RVN_MIN(_rf_mag, frame_mag) / mx);
    }
    /* ---------------------------------------------------------------------- */
    void NeuronFilter::process(YUYVImagePacket* packet, length_t bytes)
    {
        //printf("[NEURON]: got packet\n");
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
                    ifs.read(reinterpret_cast<char*>(_rf), width*height);
                    if (ifs)
                    {
                        success = true;

                        // pre-calculate the mean and 2-norm of the rf
                        _rf_mag = two_norm(_rf, width*height, _rf_mean);
                    }
                }
            }
        }

        ifs.close();
        return success;
    }
    /* ---------------------------------------------------------------------- */
    void NeuronFilter::forward_loop()
    {
        BoolPacket packet(true);

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
                //const float time = _clock.now();

                FloatPacket* ptr = pop_queue(_qout);
                release_flag(_qout_busy);

                if (ptr->data() > _threshold)
                {
                    // process input? or forward to audio thread? or should this
                    // all happen in the audio thread?
                    //printf("[NEURON]: spike \"%f\" @ %f\n", ptr->data(), time);
                    send_sink(&packet, 1);

                    // increase threshold by 10%
                    _threshold += (1.0f - _threshold) * _dthreshold;
                }
                else
                {
                    // printf("[NEURON]: no spike - %f\n", ptr->data());
                    // decrease threshold by 10%
                    _threshold *= (1.0f - _dthreshold);
                }

                // printf("[NEURON]: thr = %f\n", _threshold);

                ptr->set_data(0.0f);

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
