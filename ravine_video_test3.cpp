#include <thread>
#include <chrono>

#include "ravine_utils.hpp"
#include "ravine_packets.hpp"
#include "ravine_video_source.hpp"
#include "ravine_event_source.hpp"
#include "ravine_audio_filter.hpp"
#include "ravine_neuron_filter.hpp"
#include "ravine_datafile_sink.hpp"

#define FRAMERATE 15

#define MICROSECONDS 1000000
#define WIDTH 320
#define HEIGHT 240
#define LEFT 96 //WIDTH - 64
#define TOP  56 //HEIGHT - 64
/* ========================================================================= */
int main(int narg, const char** args)
{
    const char* dev = "/dev/video0";

    if (narg < 2)
    {
        printf("[ERROR]: not port given!\n");
        printf("Usage: %s <host_port::Int>\n", args[0]);
        return -1;
    }

    int port = std::atoi(args[1]);

    if (port < 1 || port > 65535)
    {
        printf("[ERROR]: invalid port %d\n", port);
        return -1;
    }

    RVN::AudioFilter audio;

    RVN::V4L2 video(dev, WIDTH, HEIGHT, FRAMERATE);

    if (!video.open_stream())
    {
        printf("[ERROR]: failed to open stream\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        return -1;
    }

    if (!video.initialize_buffers(16))
    {
        printf("[ERROR]: failed to init buffers\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        return -1;
    }

    RVN::NeuronFilter neuron("/home/pi/projects/RaViNE/assets/rf-05.pgm", LEFT, TOP, 8);

    if (!neuron.isvalid())
    {
        printf("[ERROR]: failed to initialize neuron filter\n");
        printf("    [MSG]: %s\n", neuron.get_error_msg().c_str());
        return -1;
    }

    RVN::DataFileSink datafile("./testfile.rdf", audio.frames_per_buffer);

    if (!datafile.isvalid())
    {
        printf("[ERROR]: failed to init sink\n");
        printf("[MSG]: %s\n", datafile.get_error_msg().c_str());
        return -1;
    }

    //RVN::EventSource events(port);

    //// prep the event source -> datafile sink stream
    //events.register_sink(&datafile);

    //if (!events.has_valid_sink())
    //{
        //printf("[ERROR]: failed to register sink for event source\n");
        //return -1;
    //}

    //// start accepting connections asap!
    //if (!events.open_stream())
    //{
        //printf("[ERROR]: Failed to open stream\n");
        //return -1;
    //}

    // while we wait for event_source connection, set up audio system:
    // register the file sink with the audio filter / source
    audio.register_sink(&datafile);

    if (!audio.has_valid_sink())
    {
        printf("[ERROR]: failed to register sink for audio filter\n");
        return -1;
    }

    neuron.register_sink(&audio);

    if (!neuron.has_valid_sink())
    {
        printf("[ERROR]: failed to register sink with source\n");
        return -1;
    }

    video.register_sink(&neuron);

    if (!video.has_valid_sink())
    {
        printf("[ERROR]: failed to register sink with source\n");
        return -1;
    }

    printf("[MAIN]: everything seems to be working...\n");

    //if (!events.start_stream())
    //{
        //printf("[ERROR]: failed to start event stream\n");
        //return -1;
    //}

    ////all ready (hopefully..?) wait for connection from our external event
    ////generator
    //while (!events.isready())
    //{
        //RVN::sleep_ms(10);
    //}

    if (!video.start_stream())
    {
        printf("[ERROR]: failed to start stream\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        return -1;
    }

    printf("[MAIN]: entering main loop!\n");
    //while (events.still_running())
    //{
        //RVN::sleep_ms(100);
    //}

    RVN::sleep_ms(30000);

    printf("[MAIN]: exiting main loop!\n");

    printf("[MAIN]: stopping video stream!\n");
    if (!video.stop_stream())
    {
        printf("[ERROR]: error during streaming\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        video.close_stream();
        return -1;
    }

    printf("[MAIN]: closing video stream!\n");
    if (!video.close_stream())
    {
        printf("[ERROR]: failed to close stream?\n");
        return -1;
    }

    //printf("[MAIN]: waiting for event stream to receive close signal\n");
    //while (events.still_running())
    //{
        //RVN::sleep_ms(500);
    //}

    //printf("[MAIN]: closing event stream!\n");
    //if (!events.close_stream())
    //{
        //printf("[ERROR]: Failed to close event stream\n");
        //return -1;
    //}

    return 0;
}

