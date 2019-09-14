#include <iostream>
#include <cstdlib>

extern "C"
{
#include "portaudio.h"
#include "pa_util.h"
}

#include "ravine_utils.hpp"
#include "ravine_event_source.hpp"
#include "ravine_audio_filter.hpp"
#include "ravine_datafile_sink.hpp"

int main(int narg, const char** args)
{
    if (narg < 2)
    {
        std::cout << "[ERROR]: not port given!" << std::endl;
        std::cout << "Usage: " << args[0] << " <host_port::Int> " << std::endl;
        return -1;
    }

    int port = std::atoi(args[1]);

    if (port < 1 || port > 65535)
    {
        std::cout << "[ERROR]: invalid port " << port << std::endl;
        return -1;
    }

    RVN::DataFileSink sink("./testfile.rdf", filter.frames_per_buffer);

    if (!sink.isvalid())
    {
        std::cout << "[ERROR]: failed to init sink" << std::endl;
        std::cout << "[MSG]: " << sink.get_error_msg() << std::endl;
        return -1;
    }

    RVN::EventSource event_source(port);

    // prep the event source -> datafile sink stream
    event_source.register_sink(&sink);
    if (!event_source.has_valid_sink())
    {
        std::cout << "[ERROR]: failed to register sink for event source" << std::endl;
        return -1;
    }

    // start accepting connections asap!
    if (!event_source.open_stream())
    {
        std::cout << "[ERROR]: Failed to open stream" << std::endl;
        return -1;
    }

    RVN::AudioFilter filter;

    // while we wait for event_source connection, set up audio system:
    // register the file sink with the audio filter / source
    filter.register_sink(&sink);

    if (!filter.has_valid_sink())
    {
        std::cout << "[ERROR]: failed to register sink for audio filter" << std::endl;
        return -1;
    }

    std::cout << "All appears ok... opening audio stream" << std::endl;

    if (!filter.open_stream())
    {
        std::cout << "[ERROR]: failed to open audio stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        return -1;
    }

    if (!filter.start_stream())
    {
        std::cout << "[ERROR]: failed to start audio stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        filter.close_stream();
        return -1;
    }

    // now start the event_source (which just "starts" the datafile sink stream,
    // which has already been started by filter.start_stream() above...)
    (void)event_source.start_stream();

    // all ready (hopefully..?) wait for connection from our external event
    // generator
    while (!event_source.isready())
    {
        RVN::sleep_ms(100);
    }

    // generate some pink noise data...
    RVN::sleep_ms(2000);

    //for (int k = 0; k < 4; ++k)
    //{
        //RVN::sleep_ms(250);
        //filter.send_spike();
    //}

    //RVN::sleep_ms(250);

    if (!filter.stop_stream())
    {
        std::cout << "[ERROR]: failed to stop audio stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
        filter.close_stream();
    }

    else if (!filter.close_stream())
    {
        std::cout << "[ERROR]: failed to close audio stream" << std::endl;
        std::cout << "[MSG]: " << filter.get_error_msg() << std::endl;
    }

    std::cout << "[INFO]: waiting for shutdown notice from event source" << std::endl;
    while (event_source.still_running())
    {
        RVN::sleep_ms(500);
    }

    if (!event_source.close_stream())
    {
        std::cout << "[ERROR]: Failed to close event stream" << std::endl;
        return -1;
    }
}
