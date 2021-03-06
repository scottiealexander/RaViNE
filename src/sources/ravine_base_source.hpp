#ifndef RAVIE_BASE_SOURCE_HPP_
#define RAVIE_BASE_SOURCE_HPP_

#include "ravine_base_sink.hpp"

namespace RVN
{
    template <class PacketType>
    class Source
    {
    public:
        virtual ~Source() {}
        virtual bool open_stream() = 0;
        virtual bool start_stream() = 0;
        virtual bool stop_stream() = 0;
        virtual bool close_stream() = 0;

        inline Sink<PacketType>& operator>>(Sink<PacketType>* sink)
        {
            register_sink(sink);
            return *sink;
        }

        inline void register_sink(Sink<PacketType>* sink)
        {
            if (!has_valid_sink())
            {
                _sink = sink;
            }
        }

        inline bool has_valid_sink() { return _sink != nullptr; }

    protected:
        inline void send_sink(PacketType* packet, uint32_t bytes)
        {
            if (has_valid_sink())
            {
                _sink->process(packet, bytes);
            }
        }

        inline bool open_sink_stream()
        {
            if (has_valid_sink()) { return _sink->open_stream(); }
            return true;
        }

        inline bool close_sink_stream()
        {
            if (has_valid_sink()) { return _sink->close_stream(); }
            return true;
        }

    protected:
        Sink<PacketType>* _sink = nullptr;
    };
}

#endif
