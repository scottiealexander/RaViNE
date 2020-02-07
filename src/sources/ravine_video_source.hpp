#ifndef RAVINE_VIDEO_SOURCE_HPP_
#define RAVINE_VIDEO_SOURCE_HPP_

#include <string>
#include <vector>
#include <cstring>

#include <atomic>
#include <thread>

#include "ravine_packets.hpp"
#include "ravine_base_sink.hpp"
#include "ravine_base_source.hpp"

// conversion factor b/t milliseconds and v4l2 100us exposure units
#define V4L2_TIME_MS 10
#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

namespace RVN
{
    int xioctl(int fh, unsigned long request, void *arg);
    /* =======================================================================*/
    class MMBuffer : public YUYVImagePacket
    {
    public:
        MMBuffer() {}
        MMBuffer(int, uint32_t, uint32_t);
        ~MMBuffer();
        void operator=(const RVN::MMBuffer&);

        inline void set_width(int width) { this->_width = width; }
    };

    using BufferVec = std::vector<MMBuffer*>;
    /* =======================================================================*/
    class V4L2 : public Source<YUYVImagePacket>
    {
    public:
        V4L2(const char*, int, int, int);
        ~V4L2();

        bool open_stream() override;
        bool start_stream() override;
        bool stop_stream() override;
        bool close_stream() override;

        bool initialize_buffers(int count = 8);

        inline const std::string& get_error_msg() const { return _err_msg; }
        inline bool isvalid() const { return _isvalid; }
        inline int buffer_count() const { return _buffers.size(); }

        inline int get_fd() const { return _fd; }

        bool set_hardware_crop(int left, int top, int width, int height);
        bool set_hardware_crop(const CropWindow& win);

    private:
        bool verify_capabilities();
        bool set_pixel_format();
        bool set_framerate(int, float&);
        bool set_exposure(float);

        void stream();

        inline void set_error_msg(const std::string& msg)
        {
            if (isvalid())
            {
                _err_msg = msg;
                _isvalid = false;
            }
            else
            {
                _err_msg.append(" " + msg);
            }
        }

        inline void reset_error_state()
        {
            _err_msg.clear();
            _isvalid = true;
        }

        inline bool persist()
        {
            return _state_continue.test_and_set(std::memory_order_acquire);
        }

        inline void send_stop() { _state_continue.clear(); }

    private:
        const std::string _dev;
        int _fd;

        int _width;
        int _height;
        int _framerate;

        bool _isvalid;
        std::string _err_msg;

        BufferVec _buffers;

        std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;
        std::thread _stream_thread;
    };
    /* =======================================================================*/
}
#endif
