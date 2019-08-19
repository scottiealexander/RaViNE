#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <iostream>

#include <linux/v4l2-controls.h>
#include <linux/v4l2-common.h>
#include <linux/videodev2.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <string.h>
#include <time.h>

// convert 100us units to milliseconds
#define V4L2_TIME_MS 10

#define MICROSECONDS 1000000
#define WIDTH 640
#define HEIGHT 360

using namespace std;
/* ========================================================================= */
void normalize(uint8_t* buffer, int32_t width, int32_t height)
{
    float mx = 0.0f;
    float mn = 255.0f;
    for(int32_t k = 0; k < (width*height); ++k)
    {
        float tmp = (float)buffer[k];
        if (tmp > mx)
        {
            mx = tmp;
        }
        else if (tmp < mn)
        {
            mn = tmp;
        }
    }
    
    mx -= mn;
    
    if (mx > 0.0f)
    {
        for(int32_t k = 0; k < (width*height); ++k)
        {
            buffer[k] = (uint8_t)((((float)buffer[k] - mn) / mx) * 255.0f);
        }
    }
    
}
/* ========================================================================= */
void yuv422_to_gray(uint8_t* buffer, int32_t width, int32_t height, uint8_t* obuffer)
{

    for (int32_t k = 0; k < (width*height); ++k)
    {
        //low byte of every other 16-bit sample is y-channel
        obuffer[k] = buffer[k*2];
    }
}
/* ========================================================================= */
int write_pgm(uint8_t* buffer, int32_t width, int32_t height, const char* ofile)
{
    FILE* fp = fopen(ofile, "wb");
    
    if (fp == NULL)
    {
        return 1;
    }
    
    fprintf(fp, "P5\n%d %d\n255\n", width, height);
    fwrite(buffer, sizeof(uint8_t), (width*height), fp);
    fclose(fp);
    
    return 0;
}
/* ========================================================================= */
int main()
{
    // 1.  Open the device
    int fd; // A file descriptor to the video device
    fd = open("/dev/video2", O_RDWR);
    
    if(fd < 0)
    {
        perror("Failed to open device, OPEN");
        return 1;
    }


    // 2. Ask the device if it can capture frames
    v4l2_capability capability;
    if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0)
    {
        // something went wrong... exit
        perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
        return 1;
    }
    
    v4l2_control c;
    c.id = V4L2_CID_EXPOSURE_AUTO;
    c.value = V4L2_EXPOSURE_MANUAL;
    
    if(ioctl(fd, VIDIOC_S_CTRL, &c) < 0)
    {
        perror("Failed to set exposure state");
        return 1;
    }
    
    c.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    c.value = 50 * V4L2_TIME_MS;
    if(ioctl(fd, VIDIOC_S_CTRL, &c) < 0)
    {
        perror("Failed to set exposure value");
        return 1;
    }

    // 3. Set Image format
    v4l2_format imageFormat;
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = WIDTH;
    imageFormat.fmt.pix.height = HEIGHT;
    
    // imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //YUV 4:2:2 [Y0,U0,Y1,V0]
    
    
    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
    
    // tell the device you are using this format
    if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0)
    {
        perror("Device could not set format, VIDIOC_S_FMT");
        return 1;
    }


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


    // 5. Quety the buffer to get raw data ie. ask for the you requested buffer
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
            sprintf(fname, "./frame-%03d.pgm", k);
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