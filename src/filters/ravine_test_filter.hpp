#ifndef RAVIE_TEST_FILTER_HPP_
#define RAVIE_TEST_FILTER_HPP_

#include <cstdio>

#include "ravine_packets.hpp"
#include "ravine_base_sink.hpp"
#include "ravine_base_source.hpp"
#include "ravine_base_filter.hpp"

namespace RVN
{
    class TestFilter : public Filter<YUYVImagePacket, YUYVImagePacket>
    {
    public:
        bool open_stream() override { return _sink->open_stream(); }
        bool close_stream() override { return _sink->close_stream(); }

        bool start_stream() override { return true; }
        bool stop_stream() override { return true; }

        inline void process(YUYVImagePacket* packet, length_t bytes)
        {
            printf("    [INFO]: re-forwarding buffer...\n");
            send_sink(packet, bytes);
        }

    };
}

#endif
