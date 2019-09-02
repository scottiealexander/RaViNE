#include "ravine_packets.hpp"

namespace RVN
{
    // force compiler to instantiate FramePacket for uint8_t type
    template class FramePacket<uint8_t>;
}
