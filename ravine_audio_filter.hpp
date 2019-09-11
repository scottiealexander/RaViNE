#ifndef RAVINE_AUDIO_FILTER_HPP_
#define RAVINE_AUDIO_FILTER_HPP_

#include <atomic>
#include <string>
#include <ctime>

extern "C"
{
#include "portaudio.h"
}

#include "ravine_filter_base.hpp"
#include "ravine_pink_noise.hpp"
#include "ravine_spike_waveform.hpp"

namespace RVN
{
    class AudioFilter : public Filter<FloatPacket, AudioPacket>
    {
    public:
        AudioFilter();
        ~AudioFilter();

        bool open_stream() override;
        bool start_stream() override;
        bool stop_stream() override;
        bool close_stream() override;

        void process(FloatPacket*, length_t) override;

        inline void send_spike() { _no_spike.clear(); }

        // only returns true iff send_spike() was just called with *NO*
        // intervening calls to have_spike()
        inline bool have_spike() { return _no_spike.test_and_set() == false; }

        inline bool isvalid() const { return _isvalid; }

        const std::string& get_error_msg() const { return _err_msg; }

    private:
        inline void set_error_msg(const char* msg)
        {
            set_error_msg(std::string(msg));
        }

        inline void set_error_msg(std::string&& msg)
        {
            _err_msg.assign(msg);
            _isvalid = false;
        }

        bool error_check(PaError, PaError err = paNoError);

        int callback(void*, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags);

        static int static_callback(
            const void* /* in */,
            void* out,
            unsigned long /* fpb */,
            const PaStreamCallbackTimeInfo* time,
            PaStreamCallbackFlags status,
            void* self
        )
        {
            return static_cast<AudioFilter*>(self)->callback(out, time, status);
        }

        inline float get_time() const
        {
            timespec t;
            clock_gettime(CLOCK_MONOTONIC, &t);
            return (float)t.tv_sec + ((float)t.tv_nsec) * 1e-9;
        }

        inline float get_elapsed() const
        {
            return get_time() - _start_time;
        }

    private:
        bool _isvalid;
        std::string _err_msg;

        bool _stream_open = false;
        bool _isspiking = false;

        PaStream* _pa_stream = nullptr;

        WaveForm _waveform;
        PinkNoise _noise;

        std::atomic_flag _no_spike = ATOMIC_FLAG_INIT;

        float _start_time;

    public:
        static constexpr int sample_rate = 24000; //44100;
        static constexpr int frames_per_buffer = 64; //~1.5ms @ 44100Hz
        static constexpr float output_latency = 0.030f; // 30ms, avoids choppy sound
    };
}

#endif
