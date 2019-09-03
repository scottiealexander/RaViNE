#include <thread>
#include <limits>
#include <cstdio>
#include <cmath>

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

#define DEBUG

#ifdef DEBUG
#include "ravine_image_utils.hpp"
#endif

#include "ravine_utils.hpp"
#include "ravine_video_source.hpp"

namespace RVN
{
    /* ====================================================================== */
    int xioctl(int fh, int request, void *arg)
    {
        int r;

        do
        {
            r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
    }
    /* ====================================================================== */
    MMBuffer::MMBuffer(int fd, uint32_t offset, uint32_t length)
    {
        // only memmap if the base class (RVN::YUYVImagePacket) can hold a
        // buffer of this size, in practive this will never fail as webcam
        // frames won't exceede int32_t::max (2^31-1 > 2e9) in bytes-per-frame,
        // but if it does the buffer will be in a valid but empty state, so
        // check is_empty()
        if (length <= std::numeric_limits<length_t>::max())
        {
            void* ptr = mmap(
                nullptr,                // start anywhere
                length,
                PROT_READ | PROT_WRITE, // required
                MAP_SHARED,             // recommended
                fd,
                offset
            );

            if (ptr != MAP_FAILED)
            {
                _data = (uint8_t*)ptr;
                _length = (length_t)length;
            }
            else
            {
                printf("MMAP failed\n");
            }
        }
    }
    /* ---------------------------------------------------------------------- */
    void MMBuffer::operator=(const RVN::MMBuffer& other)
    {
        // buffers do not own their _data, so we can safely move construct
        // from a temporary instance
        this->_data = other.data();
        this->_length = other.length();
        this->_width = other.width();
    }
    /* ---------------------------------------------------------------------- */
    MMBuffer::~MMBuffer()
    {
        if (_data != nullptr)
        {
            printf("[INFO]: un-mapping buffer\n");
            munmap((void*)_data, _length);
        }
    }

    /* ====================================================================== */
    V4L2::V4L2(const char* dev, int width, int height, int framerate) :
        _dev(dev),
        _width(width),
        _height(height),
        _framerate(framerate),
        _isvalid(true) {}
    /* ---------------------------------------------------------------------- */
    V4L2::~V4L2()
    {
        for (int k = 0; k < _buffers.size(); ++k)
        {
            if (_buffers[k] != nullptr)
            {
                printf("[INFO]: deleteing buffer pointer\n");
                delete _buffers[k];
            }
        }
        _buffers.clear();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::open_stream()
    {
        _fd = open(_dev.c_str(), O_RDWR | O_NONBLOCK, 0);

        if (_fd < 0)
        {
            set_error_msg("Failed to open device");
        }
        else if (verify_capabilities() && set_pixel_format())
        {

            // set the framerate as requested, driver may pick a different frame
            // rate however (<act> below)
            float actual_fps = 0.0f;

            if (set_framerate(_framerate, actual_fps) || actual_fps > 0.0f)
            {
                if (actual_fps != (float)_framerate)
                {
                    printf("***********************************************\n");
                    printf("[WARNING]: failed to set framerate to %d fps",
                        _framerate);
                    printf("           using %f fps instead\n",
                        actual_fps);
                    printf("***********************************************\n");

                    // set_framerate sets the error state to true if the driver
                    // chose a different framerate, we must reset to be able to
                    // to set the exposure
                    reset_error_state();
                }

                set_exposure(actual_fps);
            }
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::verify_capabilities()
    {
        if (!isvalid()) { return false; }

        v4l2_capability cap = {};

        // make sure the device can capture  / stream images / video
        if(xioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            set_error_msg("Failed to get device capabilities");
        }
        else if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
            set_error_msg("Device doesn't support video capture");
        }
        else if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            set_error_msg("Device doesn't support video streaming");
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::set_pixel_format()
    {
        if (!isvalid()) { return false; }

        v4l2_format fmt = {};

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = _width;
        fmt.fmt.pix.height = _height;

        // YUV 4:2:2 [Y0,U0,Y1,V0]
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;

        // set / validate the requested image size
        if(xioctl(_fd, VIDIOC_S_FMT, &fmt) < 0)
        {
            set_error_msg("Failed to set pixel format");
        }
        else
        {
            // incase the driver is forcing a different size
            _width = fmt.fmt.pix.width;
            _height = fmt.fmt.pix.height;
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::set_framerate(int req, float& act)
    {
        if (!isvalid()) { return false; }

        act = -1.0f;

        v4l2_streamparm streamparm = {};

        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // see if the camera allows setting time-per-frame and if so set it
        if (xioctl(_fd, VIDIOC_G_PARM, &streamparm) < 0)
        {
            set_error_msg("Failed to get stream param");
        }
        else if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)
        {
            auto& tpf = streamparm.parm.capture.timeperframe;

            // framerate is set as frame interval, so 1 / rate
            tpf.numerator = 1;
            tpf.denominator = req;

            if (xioctl(_fd, VIDIOC_S_PARM, &streamparm) < 0)
            {
                set_error_msg("Failed to set stream param");
            }
            else if (tpf.denominator != (uint32_t)req)
            {
                set_error_msg("Driver rejected framerate set request");
                act = ((float)tpf.denominator) / ((float)tpf.numerator);
            }
            else
            {
                // we got the framerate we asked for
                act = (float)req;
            }
        }
        else
        {
            set_error_msg("Drive doesn't allow setting framerate");
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::set_exposure(float framerate)
    {
        if (!isvalid()) { return false; }

        v4l2_control c = {};
        c.id = V4L2_CID_EXPOSURE_AUTO;
        c.value = V4L2_EXPOSURE_MANUAL;

        if(xioctl(_fd, VIDIOC_S_CTRL, &c) < 0)
        {
            set_error_msg("Failed to set exposure state to manual");
        }
        else
        {
            // per-frame exposure in v4l2 100us units,
            // set to framerate limit - 2ms:
            //  * convert framerate (Hz) to ms-per-frame (- 2)
            //  * convert to 100us units
            int exposure = ((int)floor(1000.0f / framerate) - 2) * V4L2_TIME_MS;

            c.id = V4L2_CID_EXPOSURE_ABSOLUTE;
            c.value = exposure;

            if(xioctl(_fd, VIDIOC_S_CTRL, &c) < 0)
            {
                set_error_msg("Failed to set exposure duration");
            }
            else
            {
                // double check that the driver accepted our value
                CLEAR(c);
                c.id = V4L2_CID_EXPOSURE_ABSOLUTE;

                if (xioctl(_fd, VIDIOC_G_CTRL, &c) == 0)
                {
                    if (c.value != exposure)
                    {
                        set_error_msg("Drive rejected exposure duration");
                    }
                }
                else
                {
                    set_error_msg("Failed to query exposure duration");
                }
            }
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::set_hardware_crop(const CropWindow& win)
    {
        return set_hardware_crop(win.col, win.row, win.width, win.height);
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::set_hardware_crop(int left, int top, int width, int height)
    {
        v4l2_cropcap cap = {};
        cap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (xioctl(_fd, VIDIOC_CROPCAP, &cap) < 0)
        {
            if (errno == ENODATA)
            {
                set_error_msg("Camera does NOT support hardware cropping");
            }
            else
            {
                set_error_msg("Failed to query cropping capability");
            }
        }
        else
        {
            v4l2_crop crop = {};
            crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            // query the default cropping setting to get the crop bounds
            if (xioctl(_fd, VIDIOC_G_CROP, &crop) < 0)
            {
                if (errno == EINVAL)
                {
                    // this should never happen, given the above, right?
                    set_error_msg("Camera does NOT support hardware cropping");
                }
                else
                {
                    set_error_msg("Failed to get cropping params");
                }
            }
            else
            {
                // make sure our requested rect lies within the devices crop
                // bounds
                crop.c.left = RVN_MAX(left, cap.bounds.left);
                crop.c.top = RVN_MAX(top, cap.bounds.top);
                crop.c.width = RVN_MIN((uint32_t)width, cap.bounds.width);
                crop.c.height = RVN_MIN((uint32_t)height, cap.bounds.height);

                if (xioctl(_fd, VIDIOC_S_CROP, &crop) < 0)
                {
                    set_error_msg("Failed to set cropping params");
                }
                else
                {
                    // device is set to crop, so keep track of our new image
                    // size (width in particular, as the buffers need that info)
                    _width = crop.c.width;
                    _height = crop.c.height;
                }
            }
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::initialize_buffers(int count)
    {
        if (!isvalid()) { return false; }

        // ask the device to initilize buffer for capture streaming
        v4l2_requestbuffers req = {};

        req.count = count;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        // make sure enough buffers were actually initialized
        if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
        {
            set_error_msg("Could not request buffer from device");
        }
        else if (req.count > 1)
        {
            _buffers.resize(req.count);
            for (int k = 0; k < req.count; ++k)
            {
                _buffers[k] = nullptr;
            }

            // memory-map each buffer
            for (int k = 0; k < req.count; ++k)
            {
                v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = k;

                if (xioctl(_fd, VIDIOC_QUERYBUF, &buf) < 0)
                {
                    set_error_msg("Failed to query buffers");
                    break;
                }

                _buffers[k] = new MMBuffer(_fd, buf.m.offset, buf.length);
                //_buffers.push_back(new MMBuffer(_fd, buf.m.offset, buf.length));


                if (_buffers[k]->is_empty())
                {
                    set_error_msg("Failed to mmap buffer");
                    break;
                }
                else
                {
                    // the buffer needs to know it's own width (in pixels)
                    // as it is the packet that we'll send to our sink, and sink
                    // requires that information
                    _buffers[k]->set_width(_width);
                }

            }
        }
        else
        {
            set_error_msg("Insufficient memory on device");
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::start_stream()
    {
        if (!isvalid()) { return false; }

        if (buffer_count() < 1)
        {
            set_error_msg("Buffers have not been initialized!");
            return false;
        }

        // queue up all the buffers
        for (int k = 0; k < buffer_count(); ++k)
        {
            v4l2_buffer buf = {};

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = k;

            if (xioctl(_fd, VIDIOC_QBUF, &buf) < 0)
            {
                set_error_msg("Failed to Q buffer");
                break;
            }
        }

        printf("[INFO]: buffers appear to be queued\n");

        if (isvalid())
        {
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (xioctl(_fd, VIDIOC_STREAMON, &type) < 0)
            {
                set_error_msg("Failed to start streaming");
            }
            else
            {
                printf("[INFO]: launching stream\n");

                // by calling persist() we set the _state_continue flag to true
                // (reguardless of it's current state which should start false)
                // so that V4L2::stream will wait for the stop signal (seting
                // _state_continue to false)
                persist();

                // launch the actual frame stream
                _stream_thread = std::thread(&V4L2::stream, this);
            }
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    void V4L2::stream()
    {
        if (!isvalid()) { return; }

        fd_set fds;

        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

#ifndef DEBUG
        if (!_sink->open_stream())
        {
            set_error_msg("Failed to open sink stream");
            return;
        }
#else
        printf("[INFO]: allocating copy gray buffer, %d x %d\n", _width, _height);
        uint8_t* gray = new uint8_t[_width*_height];
        printf("[INFO]: success allocating gray\n");
        int kframe = 0;
#endif
        bool error = false;
        std::string err_msg;

        printf("[INFO]: entering main stream loop\n");
        while (persist() && (!error) && (kframe < 5))
        {
            bool frame_ready = false;

            printf("[INFO]: waiting for next frame\n");
            while (!frame_ready)
            {
                FD_ZERO(&fds);
                FD_SET(_fd, &fds);

                int r = select(_fd + 1, &fds, NULL, NULL, &timeout);

                if (r == 0)
                {
                    // timed out
                    error = true;
                    err_msg = "Select timed out waiting for frame";
                    break;
                }
                else if (r == -1 && errno != EINTR)
                {
                    // something went wrong
                    error = true;
                    err_msg = "Error occured in select!";
                    break;
                }
                else if (r > 0)
                {
                    frame_ready = true;
                    break;
                }

                // sleep for a bit
                sleep_ms(10);
            }

            if (frame_ready)
            {
                printf("[INFO]: got frame %d\n", kframe);

                v4l2_buffer buf = {};

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (xioctl(_fd, VIDIOC_DQBUF, &buf) < 0)
                {
                    //failed to dequeue frame
                    if (errno != EAGAIN)
                    {
                        error = true;
                        err_msg = "Failed to dqueue frame";
                    }

                }
                else if (buf.index < _buffers.size())
                {
                    printf("[INFO]: appears we have a valid buffer at %d\n", buf.index);

                    // in a YUYV frame, every other sample is luminance, so
                    // total bytes is width x height x 2, so we send width and
                    // bytesused
#ifndef DEBUG
                    // send to sink (this should be synchronous but fast)
                    //send_sink(_buffers[buf.index]);
#else
                    printf("[INFO]: converting yuyv to gray\n");
                    if (_buffers[buf.index]->data() == nullptr)
                    {
                        printf("[ERROR]: buffer data appear to be null\n");
                    }
                    else
                    {
                        yuv422_to_gray(_buffers[buf.index]->data(), _width, _height, gray);
                        normalize(gray, _width, _height);

                        printf("[INFO]: writing frame to file\n");
                        char ofile[32] = {'\0'};
                        sprintf(ofile, "./frames/frame-%03d.pgm", kframe);
                        write_pgm(gray, _width, _height, ofile);
                        ++kframe;
                    }
#endif

                    if (xioctl(_fd, VIDIOC_QBUF, &buf) < 0)
                    {
                        // NOTE TODO FIXME: what do we do here?
                        // failed to re-queue the frame
                        error = true;
                        err_msg = "failed to re-queue frame";
                    }
                }
            }
        }

        if (error)
        {
            printf("[INFO]: an error occured during streaming, will report\n");
            set_error_msg(err_msg);
        }
        else
        {
            printf("[INFO]: ending stream\n");
        }

#ifndef DEBUG
        //_sink->close_stream();
#endif
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::stop_stream()
    {
        send_stop();
        _stream_thread.join();

        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (xioctl(_fd, VIDIOC_STREAMOFF, &type) < 0)
        {
            set_error_msg("Failed to stop stream");
        }

        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool V4L2::close_stream()
    {
        close(_fd);
        return true;
    }
    /* ====================================================================== */
}
