#include <thread>
#include <chrono>

#include "ravine_utils.hpp"
#include "ravine_packets.hpp"
#include "ravine_video_source.hpp"
#include "ravine_audio_filter.hpp"
#include "ravine_neuron_filter.hpp"

#define FRAMERATE 15

#define MICROSECONDS 1000000
#define WIDTH 320
#define HEIGHT 240
#define LEFT 288 //WIDTH - 32
#define TOP  208 //HEIGHT - 32
/* ========================================================================= */
int main()
{
    const char* dev = "/dev/video0";

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

    RVN::NeuronFilter neuron("/home/pi/projects/RaViNE/assets/rf-01.pgm", LEFT, TOP, 8);

    if (!neuron.isvalid())
    {
        printf("[ERROR]: failed to initialize neuron filter\n");
        printf("    [MSG]: %s\n", neuron.get_error_msg().c_str());
        return -1;
    }

    RVN::AudioFilter audio;

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

    printf("[OK]: everything seems to be working...\n");
    //return 0;

    if (!video.start_stream())
    {
        printf("[ERROR]: failed to start stream\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        return -1;
    }

    RVN::sleep_ms(15000);

    if (!video.stop_stream())
    {
        printf("[ERROR]: error during streaming\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        video.close_stream();
        return -1;
    }

    if (!video.close_stream())
    {
        printf("[ERROR]: failed to close stream?\n");
        return -1;
    }

    return 0;
}

