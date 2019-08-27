#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/videodev2.h>


#include "ravine_video_utils.hpp"

/* ========================================================================== */
MMBuffer::MMBuffer(const int fd, const size_t offset, const size_t length) :
    length(length), valid(false)
{
    start = mmap(nullptr,       // start anywhere
        length,
        PROT_READ | PROT_WRITE  // required
        MAP_SHARED,             // recommended
        fd,
        offset
    );
    
    valid = start != MAP_FAILED;
}
/* -------------------------------------------------------------------------- */
MMBuffer::~MMBuffer()
{
    if (valid)
    {
        munmap(start, length);
    }
}

/* ========================================================================== */
V4L2::V4L2() : _isvalid(true) {}
/* -------------------------------------------------------------------------- */
bool V4L2::open_stream(const char* dev, const int width, const int height, const int framerate)
{
    bool success = true;
    _fd = open(dev, O_RDWR | O_NONBLOCK, 0);
    
    if (_fd < 0)
    {
        set_error_msg("Failed to open device");
        success = false;
    }
    else
    {
        v4l2_capability cap = {0};
        
        // make sure the device can capture  / stream images / video
        if(ioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            set_error_msg("Failed to get device capabilities");
            success = false;
        }
        else
        {
            if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
            {
                set_error_msg("Device doesn't support video capture");
                success = false;
            }
            else if (!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                set_error_msg("Device doesn't support video streaming");
                success = false;
            }
            else
            {
                v4l2_format fmt = {0};
                
                fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                fmt.fmt.pix.width = width;
                fmt.fmt.pix.height = height;
                
                // YUV 4:2:2 [Y0,U0,Y1,V0]
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                
                
                fmt.fmt.pix.field = V4L2_FIELD_NONE;
                
                // set / validate the requested image size
                if(ioctl(_fd, VIDIOC_S_FMT, &fmt) < 0)
                {
                    set_error_msg("Failed to set pixel format");
                    success = false;
                }
                else
                {
                    // incase the driver is forcing a different size
                    _width = fmt.fmt.pix.width;
                    _height = fmt.fmt.pix.height;
                }
            }
        }
    
    }
    
    if (success)
    {
        float act;
        if (!set_framerate(framerate, act))
        {
            success = false;
        }
        else
        {
            // log actual framerate? or something like that?
            success = init_stream(8);
        }
    }
    
    return success;
}
/* -------------------------------------------------------------------------- */
bool V4L2::set_framerate(const int& req, float& act)
{
    if (!_isvalid) { return false; }
    
    act = -1.0f;
    
    struct v4l2_streamparm streamparm;
    
    CLEAR(streamparm);
    
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (ioctl(fd, VIDIOC_G_PARM, &streamparm) < 0)
    {
        set_error_msg("Failed to get stream param");
        return false;
    }
    
    if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
    {
        auto &tpf = streamparm.parm.capture.timeperframe;

        // framerate is set as frame interval, so 1 / rate
        tpf.numerator = 1;
        tpf.denominator = req;

        if (ioctl(fd, VIDIOC_S_PARM, &streamparm) < 0)
        {
            set_error_msg("Failed to set stream param");
            return false;
        }

        if (tpf.denominator != req)
        {
            set_error_msg("Driver rejected framerate set request");
            act = static_cast<float>(tpf.denominator) / static_cast<float>(tpf.numerator);
            return false;
        }
    }
    else
    {
        set_error_msg("Drive doesn't allow setting framerate");
        return false;
    }
    
    return true;
}
/* -------------------------------------------------------------------------- */
bool V4L2::init_stream(const int count)
{
    if (!_isvalid) { return false; }
    
    bool success = true;
    
    // ask the device to initilize buffer for capture streaming
    v4l2_requestbuffers req = {0};
    
    req.count = count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    // make sure enough buffers were actually initialized
    if(ioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
    {
        set_error_msg("Could not request buffer from device");
        success = false;
    }
    else if (req.count > 1)
    {
        _buffers.resize(req.count);

        // memory-map each buffer
        for (int k = 0; k < req.count; ++k)
        {
            v4l2_buffer buf;
    
            CLEAR(buf);
    
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = k;
    
            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
            {
                set_error_msg("Failed to query buffers");
                success = false;
                break;
            }
            
            _buffers[k] = MMBuffer(_fd, buf.m.offset, buf.length);
    
    
            if (!_buffers[k].valid)
            {
                set_error_msg("Failed to mmap buffer");
                success = false;
                break;
            }
            
        }
    }
    else
    {
        set_error_msg("Insufficient memory on device");
        success = false;
    }
    
    return success;
}
/* -------------------------------------------------------------------------- */
bool V4L2::start_stream()
{
    if (!_isvalid) { return false; }
    
    bool success = true;
    v4l2_buf_type type;

    // queue up all the buffers
    for (int k = 0; k < buffer_count(); ++k)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);
        
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = k;

        if (ioctl(_fd, VIDIOC_QBUF, &buf) < 0)
        {
            set_error_msg("Failed to Q buffer");
            success = false;
            break;
        }
    }
    
    
    if (success)
    {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        
        if (ioctl(_fd, VIDIOC_STREAMON, &type) < 0)
        {
            set_error_message("Failed to start streaming");
            success = false;
        }
    }
    
    stream();
    
    return success;
}
/* -------------------------------------------------------------------------- */
void V4L2::stream()
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(_fd, &fds);
    
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    bool error = false;
    
    while (!_to_stream.have_message() && !error)
    {
        bool frame_ready = false;
    
        while (!frame_ready)
        {
            r = select(_fd + 1, &fds, NULL, NULL, &tv);

            if (r == 0)
            {
                // timed out
                error = true;
                break;
            }
            else if (r == -1 && errno != EINTR)
            {
                // something went wrong
                error = true;
                break;
            }
            else if (r > 0)
            {
                frame_ready = true;
            }
            
            // sleep for a bit
        }
        
        if (frame_ready)
        {
            v4l2_buffer buf = {0};
            
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            
            if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
            {
                //failed to dequeue frame
                if (errno != EAGAIN)
                {
                    error = true;
                }
                
            }
            else if (buf.index < _buffers.size())
            {
                // in a YUYV frame, every other sample is luminance, so total bytes is
                // width x height x 2, so we send width, height and bytesused
                
                // send to sink (this should be synchronous but fast)
                sink.process(_buffers[buf.index].start, buf.bytesused, _width);
            }
            
            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
            {
                // failed to re-queue the frame
            }
        }
    }
}
/* -------------------------------------------------------------------------- */
bool V4L2::stop_stream()
{
    _to_stream.send_message();
    _stream_thread.join();
    return true;
}
/* -------------------------------------------------------------------------- */
bool close_stream()
{
    bool success = true;
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(_fd, VIDIOC_STREAMOFF, &type) < 0)
    {
        set_error_msg("Failed to stop stream");
        success = false;
    }
    
    close(_fd);
    
    return success;
}
/* -------------------------------------------------------------------------- */
// std::atomic_bool end;
// if (end.load())
// from main thread, end.store(true)
/* ========================================================================== */