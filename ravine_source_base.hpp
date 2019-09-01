#ifndef RAVIE_SOURCE_BASE_HPP_
#define RAVIE_SOURCE_BASE_HPP_

#include "ravine_sink_base.hpp"

namespace RVN
{
    template <class PacketType>
    class Source
    {
    public:
        virtual bool open_stream() = 0;
        virtual bool start_stream() = 0;
        virtual bool stop_stream() = 0;
        virtual bool close_stream() = 0;

        inline bool register_sink(Sink<PacketType>* sink)
        {
            if (!has_valid_sink())
            {
                _sink = sink;
            }
        }

        inline bool has_valid_sink() { return _sink != nullptr; }

    protected:
        inline void send_sink(PacketType& packet)
        {
            if (has_valid_sink())
            {
                _sink->process(packet);
            }
        }

    protected:
        Sink<PacketType>* _sink = nullptr;
    };
}

#endif
