#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <linux/v4l2-controls.h>
#include <linux/v4l2-common.h>
#include <linux/videodev2.h>

#include <string.h>
#include <time.h>

#include "ravine_video_source.hpp"
#include "ravine_image_utils.hpp"

#define FRAMERATE 15

#define MICROSECONDS 1000000
#define WIDTH 640
#define HEIGHT 360

using namespace std;

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

    int fd = source.get_fd();

    // 4. Request Buffers from the device
    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count = 1; // one request buffer
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // request a buffer wich we an use for capturing frames
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0)
    {
        perror("Could not request buffer from device, VIDIOC_REQBUFS");
        return 1;
    }


    // 5. Query the buffer to get raw data ie. ask for the you requested buffer
    // and allocate memory for it
    v4l2_buffer queryBuffer = {0};
    queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queryBuffer.memory = V4L2_MEMORY_MMAP;
    queryBuffer.index = 0;
    if(ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0)
    {
        perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
        return 1;
    }

    // use a pointer to point to the newly created buffer
    // mmap() will map the memory address of the device to
    // an address in memory
    uint8_t* buffer = (uint8_t*)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, queryBuffer.m.offset);
    memset(buffer, 0, queryBuffer.length);


    // 6. Get a frame
    // Create a new buffer type so the device knows whichbuffer we are talking about
    v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));
    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    // Activate streaming
    int type = bufferinfo.type;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("Could not start streaming, VIDIOC_STREAMON");
        return 1;
    }

/***************************** Begin looping here *********************/
    uint8_t* img = (uint8_t*)malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    float dur = 0.0f;
    int32_t nframe = 0;

    for (int32_t k = 0; k < 30; ++k)
    {
        struct timespec t1, t2;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);

        // Queue the buffer
        if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0)
        {
            printf("[INFO]: failed to queue buffer for frame %d\n", k);
            continue;
            // perror("Could not queue buffer, VIDIOC_QBUF");
            // return 1;
        }

        // Dequeue the buffer
        if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0)
        {
            printf("[INFO]: dropped frame %d\n", k);
            continue;
            // perror("Could not dequeue the buffer, VIDIOC_DQBUF");
            // return 1;
        }
        // Frames get written after dequeuing the buffer

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);

        nframe += 1;

        dur += (float)(t2.tv_sec - t1.tv_sec) * 1000.0 +
            (float)(t2.tv_nsec - t1.tv_nsec) / 1000000.0;

        if (bufferinfo.bytesused == (WIDTH*HEIGHT*sizeof(uint16_t)))
        {
            yuv422_to_gray(buffer, WIDTH, HEIGHT, img);
            normalize(img, WIDTH, HEIGHT);

            char fname[32] = {'\0'};
            sprintf(fname, "./frames/frame-%03d.pgm", k);
            write_pgm(img, WIDTH, HEIGHT, fname);
        }
        else
        {
            printf("[ERROR]: expected %d bytes, got %d\n", WIDTH*HEIGHT*2, bufferinfo.bytesused);
        }

    }

    free(img);

    cout << "Elapsed time: " << dur / nframe << " milliseconds" << endl;


/******************************** end looping here **********************/

    // end streaming
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
    {
        perror("Could not end streaming, VIDIOC_STREAMOFF");
        return 1;
    }

    close(fd);
    return 0;
}
/* ========================================================================= */
