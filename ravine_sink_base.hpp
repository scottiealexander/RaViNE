#ifndef RAVIE_SINK_BASE_HPP_
#define RAVIE_SINK_BASE_HPP_

namespace RVN
{
    template <class PacketType>
    class Sink
    {
    public:
        virtual bool open_stream() = 0;
        virtual bool close_stream() = 0;
        virtual void process(PacketType&) = 0;
    };
}

#endif
