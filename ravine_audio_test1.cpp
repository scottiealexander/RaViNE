#include <iostream>

#include "ravine_utils.hpp"
#include "ravine_audio_filter.hpp"

int main()
{

    RVN::AudioFilter filter;
    RVN::DataFileSink sink("./testfile.rdf", filter.frames_per_buffer);

    if (!sink.isvalid())
    {
        std::cout << "[ERROR]: failed to init sink" << std::endl;
        std::cout << "[MSG]: " << sink.get_error_msg() << std::endl;
        return -1;
    }

    // register!
    filter.register_sink(sink);

    if (!filter.has_valid_sink())
    {
        std::cout << "[ERROR]: failed to register sink" << std::endl;
        return -1;
    }

    if (!filter.open_stream())
    {
        std::cout << "[ERROR]: failed to open stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        return -1;
    }

    if (!filter.start_stream())
    {
        std::cout << "[ERROR]: failed to start stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        filter.close_stream();
        return -1;
    }

    for (int k = 0; k < 4; ++k)
    {
        RVN::sleep_ms(500);
        filter.send_spike();
    }

    RVN::sleep_ms(250);

    if (!filter.stop_stream())
    {
        std::cout << "[ERROR]: failed to stop stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        filter.close_stream();
        return -1;
    }

    if (!filter.close_stream())
    {
        std::cout << "[ERROR]: failed to close stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        return -1;
    }

}
