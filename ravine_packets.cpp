#include "ravine_packets.hpp"

namespace RVN
{
    // force compiler to instantiate a few Packet class that we know we need
    template class FramePacket<uint8_t>;
    template class BufferPacket<int16_t>;
}
