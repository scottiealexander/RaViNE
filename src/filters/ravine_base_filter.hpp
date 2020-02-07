#ifndef RAVIE_BASE_FILTER_HPP_
#define RAVIE_BASE_FILTER_HPP_

#include "ravine_packets.hpp"
#include "ravine_base_sink.hpp"
#include "ravine_base_source.hpp"

namespace RVN
{
    template <class IPacket, class OPacket>
    class Filter : public Sink<IPacket>, public Source<OPacket>
    {
    public:
        virtual bool open_stream() = 0;
        virtual bool start_stream() = 0;
        virtual bool stop_stream() = 0;
        virtual bool close_stream() = 0;

        virtual void process(IPacket*, length_t) = 0;
    };
}

#endif
