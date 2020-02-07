#ifndef RAVINE_BASE_PACKET_HPP_
#define RAVINE_BASE_PACKET_HPP_

namespace RVN
{
    template <class T>
    class Packet
    {
    public:
        Packet(T data) : _data(data) {}
        Packet() = default;
        inline T data() const { return _data; }
    protected:
        T _data;
    };
}

#endif
