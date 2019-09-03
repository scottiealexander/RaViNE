#ifndef RAVINE_PACKET_BASE_HPP_
#define RAVINE_PACKET_BASE_HPP_

namespace RVN
{
    template <class T>
    class Packet
    {
    public:
        Packet(T data) : _data(data) {}
        Packet() = default;
        T data() const { return _data; }
    protected:
        T _data;
    };
}

#endif

