#include <cstring>
#include "ravine_packets.hpp"

namespace RVN
{
    // force compiler to instantiate a few Packet class that we know we need
    template class FramePacket<uint8_t>;
    template class ScalarPacket<float>;

    /* ====================================================================== */
    void AudioPacket::copy_from(const AudioPacket& other) :
    {
        // it's ok for _length to be smaller than the actual buffer size, but
        // we have to limit the copy to _length elements
        const length_t len = RVN_MIN(this->length(), other.length());

        memcpy(this->_data, other.data(), len * sizeof (float));
        this->_length = len;
    }
    /* ---------------------------------------------------------------------- */
    AudioPacket::~AudioPacket()
    {
        if (_owned && this->_data != nullptr)
        {
            delete[] _data;
        }
    }
    /* ====================================================================== */
}
