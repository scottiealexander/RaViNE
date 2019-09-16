#include <cstddef>
#include <cstdio>

extern "C"
{
#include "portaudio.h"
}

#include "ravine_packets.hpp"
#include "ravine_pink_noise.hpp"
#include "ravine_spike_waveform.hpp"
#include "ravine_audio_filter.hpp"

#define NOISE_ROWS 16
#define NOISE_LEVEL 0.3f

namespace RVN
{
    /* ====================================================================== */
    AudioFilter::AudioFilter() :
        _isvalid(true), _waveform("./spike.wf"), _noise(NOISE_ROWS, NOISE_LEVEL)
    {
        (void)error_check(Pa_Initialize());

        // spike flag starts true:
        //      true = no spike
        //      false = spike
        _no_spike.test_and_set();

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
            if (isvalid())
            {
                set_error_msg(Pa_GetErrorText(got));
            }
            else
            {
                _err_msg.append(" & ");
                _err_msg.append(Pa_GetErrorText(got));
            }
        }
        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    int AudioFilter::callback(void* outp, const PaStreamCallbackTimeInfo* /* time */,
        PaStreamCallbackFlags /* status */)
    {
        // in theory we could use the streamcallbacktime, but that uses
        // clock_gettime(CLOCK_REALTIME), and we probably want a steady clock,
        // CLOCK_MONOTONIC or std::chrono::steady_clock which is what _clock
        // uses, which should aid synchronizing timestamps across threads
        const float time = _clock.now();
        float* out = static_cast<float*>(outp);

        for (int k = 0; k < frames_per_buffer; ++k)
        {
            if (have_spike()) { _isspiking = true; }

            if (_isspiking)
            {
                // next_sample() will set _isspiking to false once the spike
                // waveform has been sampled in its entirety, so we retain our
                // state w.r.t to spiking across calls from PA
                out[k] = _waveform.next_sample(_isspiking);
            }
            else
            {
                // NOTE: this was *out++ which *OF COURSE* leaves the pointer
                // pointing to the *END OF THE ARRAY* for the AudioPacket copy
                // construction below... WTF...
                out[k] = _noise.next_sample();
            }
        }

         // sink just copies data and returns
         AudioPacket packet(out, frames_per_buffer, time);
         send_sink(&packet, frames_per_buffer);

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

        return start_stream();
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::start_stream()
    {
        if (isvalid() && !_stream_open)
        {
            // tell our sink to prepare to receive data
            if (open_sink_stream())
            {
                // open the audio stream
                if (error_check(Pa_StartStream(_pa_stream)))
                {
                    printf("[AUDIO]: stream started\n");
                    _stream_open = true;
                }
            }
            else
            {
                set_error_msg("Failed to open sink stream");
            }
        }
        return isvalid() && _stream_open;
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::stop_stream()
    {
        if (error_check(Pa_StopStream(_pa_stream)))
        {
            _stream_open = false;
        }

        if (!close_sink_stream())
        {
            if (isvalid())
            {
                // don't overwrite an existing error message
                set_error_msg("Failed to close sink stream");
            }
            else
            {
                _err_msg.append(" & failed to close sink stream");
            }
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool AudioFilter::close_stream()
    {
        if (_stream_open) { (void)stop_stream(); }
        (void)close_sink_stream();
        return error_check(Pa_CloseStream(_pa_stream)) && isvalid();
    }
    /* ---------------------------------------------------------------------- */
    //void AudioFilter::process(BoolPacket* /* packet */, length_t /* bytes */)
    //{
        //// NOTE TODO FIXME WARNING ERROR: we should be receiving a BoolPacket
        //// from the neuron indicating a spike (in theory we don't even need
        //// to check, as the neuron should *ONLY* call process() to notify us
        //// of a spike...)
        //if (packet->data() > 0.0f)
        //{
            //send_spike();
        //}
    //}
    /* ====================================================================== */
}
