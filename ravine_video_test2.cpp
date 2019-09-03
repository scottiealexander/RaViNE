#include <thread>
#include <chrono>

#include "ravine_utils.hpp"
#include "ravine_video_source.hpp"

#define FRAMERATE 5

#define MICROSECONDS 1000000
#define WIDTH 360
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

    //if (source.isvalid())
    //{
        //printf("Source appears to be valid...\n");
        //return 0;
    //}

    if (!source.start_stream())
    {
        printf("[ERROR]: failed to start stream\n");
        printf("[MSG]: %s\n", source.get_error_msg().c_str());
        return -1;
    }

    sleep_ms(1000);

    if (source.isvalid())
    {
        if (!source.stop_stream())
        {
            printf("[ERROR]: failed to start stream\n");
            printf("[MSG]: %s\n", source.get_error_msg().c_str());
            return -1;
        }
    }
    else
    {
        printf("[ERROR]: error while streaming\n");
        printf("[MSG]: %s\n", source.get_error_msg().c_str());
        return -1;
    }

    if (!source.close_stream())
    {
        printf("[ERROR]: failed to close stream?\n");
        return -1;
    }

    return 0;
}
