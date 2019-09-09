#include <cstddef>
#include <cstdio>

extern "C"
{
    #include "portaudio.h"
}

#include "ravine_pink_noise.hpp"
#include "ravine_spike_waveform.hpp"
#include "ravine_audio_filter.hpp"

namespace RVN
{
    /* ====================================================================== */
    AudioFilter::AudioFilter() :
        _isvalid(true), _noise(16)
    {
        (void)error_check(Pa_Initialize());

        // spike flag starts true:
        //      true = no spike
        //      false = spike
        _no_spike.test_and_set();

        _waveform.load_waveform("./spike.wf");

        if (!_waveform.isvalid())
        {
            set_error_msg("Failed to init waveform");
        }
    }
    /* ---------------------------------------------------------------------- */
    AudioFilter::~AudioFilter()
    {
        (void)error_check(Pa_Terminate());
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::error_check(PaError got, PaError none)
    {
        if (got != none)
        {
            set_error_msg(Pa_GetErrorText(got));
        }
        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    int AudioFilter::callback(void* outp, const PaStreamCallbackTimeInfo* /* time */,
        PaStreamCallbackFlags /* status */)
    {
        float* out = static_cast<float*>(outp);

        for (int k = 0; k < frames_per_buffer; ++k)
        {
            if (have_spike()) { _isspiking = true; }

            if (_isspiking)
            {
                // next_sample() will set _isspiking to false once the spike
                // waveform has been sampled in its entirety, so we retain our
                // state w.r.t to spiking across calls from PA
                *out++ = _waveform.next_sample(_isspiking);
            }
            else
            {
                *out++ = _noise.next_sample();
            }
        }

        // // sink just copies data and returns
        // AudioPacket packet(out, frames_per_buffer);
        // send_sink(&packet, frames_per_buffer);

        return paContinue;
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::open_stream()
    {
        if (!isvalid()) { return false; }

        PaStreamParameters param;
        param.device = Pa_GetDefaultOutputDevice();

        if (param.device == paNoDevice)
        {
            set_error_msg("No output audio devices found");
        }
        else
        {
            param.channelCount = 1;
            param.hostApiSpecificStreamInfo = NULL;
            param.sampleFormat = paFloat32;
            param.suggestedLatency = output_latency;

            // use the static callback to be Pa compliant, which just forwards
            // the needed input to AudioFilter::callback() (implemented above)
            PaError err = Pa_OpenStream(
                &_pa_stream,
                NULL,
                &param,
                sample_rate,
                frames_per_buffer,
                paClipOff,
                &AudioFilter::static_callback,
                this
            );

            (void)error_check(err);
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::start_stream()
    {
        if (!isvalid()) { return false; }
        return error_check(Pa_StartStream(_pa_stream));
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::stop_stream()
    {
        if (!isvalid()) { return false; }
        return error_check(Pa_StopStream(_pa_stream));
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::close_stream()
    {
        if (!isvalid()) { return false; }
        return error_check(Pa_CloseStream(_pa_stream));
    }
    /* ---------------------------------------------------------------------- */
    void AudioFilter::process(FloatPacket* packet, length_t /* bytes */)
    {
        if (packet->data() > 0.0f)
        {
            send_spike();
        }
    }
    /* ====================================================================== */
}