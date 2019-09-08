#ifndef RAVINE_AUDIO_FILTER_HPP_
#define RAVINE_AUDIO_FILTER_HPP_

#include "ravine_filter_base.hpp"
#inlucde "ravine_pink_noise.hpp"

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

    private:
        PinkNoise _noise;
    };
}

#endif
