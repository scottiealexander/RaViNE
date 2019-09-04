#include <thread>
#include <chrono>

#include "ravine_utils.hpp"
#include "ravine_video_source.hpp"
#include "ravine_file_sink.hpp"

#define FRAMERATE 15

#define MICROSECONDS 1000000
#define WIDTH 320
#define HEIGHT 240

/* ========================================================================= */
int main()
{
    const char* dev = "/dev/video0";

    RVN::V4L2 source(dev, WIDTH, HEIGHT, FRAMERATE);

    if (!source.open_stream())
    {
        printf("[ERROR]: failed to open stream\n");
        printf("[MSG]: %s\n", source.get_error_msg().c_str());
        return -1;
    }

    if (!source.initialize_buffers(4))
    {
        printf("[ERROR]: failed to init buffers\n");
        printf("[MSG]: %s\n", source.get_error_msg().c_str());
        return -1;
    }

    RVN::FileSink sink(WIDTH, HEIGHT, 4);

    source.register_sink(&sink);

    if (!source.has_valid_sink())
    {
        printf("[ERROR]: failed to register sink with source\n");
        return -1;
    }

    if (!source.start_stream())
    {
        printf("[ERROR]: failed to start stream\n");
        printf("[MSG]: %s\n", source.get_error_msg().c_str());
        return -1;
    }

    sleep_ms(3000);

    if (!source.stop_stream())
    {
        printf("[ERROR]: error during streaming\n");
        printf("[MSG]: %s\n", source.get_error_msg().c_str());
        source.close_stream();
        return -1;
    }

    if (!source.close_stream())
    {
        printf("[ERROR]: failed to close stream?\n");
        return -1;
    }

    return 0;
}
