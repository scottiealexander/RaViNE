#ifndef RAVIE_SINK_BASE_HPP_
#define RAVIE_SINK_BASE_HPP_

#include "ravine_packets.hpp"

namespace RVN
{
    template <class PacketType>
    class Sink
    {
    public:
        virtual ~Sink() {}
        virtual bool open_stream() = 0;
        virtual bool close_stream() = 0;
        virtual void process(PacketType*, length_t) = 0;

        //inline bool isopen() const { return _isopen; }
    //protected:
        //bool _isopen;
    };
}

#endif
