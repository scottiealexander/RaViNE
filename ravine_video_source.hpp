#ifndef RAVINE_VIDEO_SOURCE_HPP_
#define RAVINE_VIDEO_SOURCE_HPP_

#include <string>
#include <vector>
#include <cstring>

#include <atomic>
#include <thread>

#include "ravine_sink_base.hpp"
#include "ravine_source_base.hpp"
#include "ravine_packets.hpp"

#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

namespace RVN
{
    int xioctl(int fh, int request, void *arg);
    /* =======================================================================*/
    class MMBuffer : public YUYVImagePacket
    {
    public:
        MMBuffer() {}
        MMBuffer(int, uint32_t, uint32_t);
        ~MMBuffer();
        inline void set_width(int width) { this->_width = width; }
    };

    using BufferVec = std::vector<MMBuffer>;
    /* =======================================================================*/
    class V4L2 : public Source<YUYVImagePacket>
    {
    public:
        V4L2(const char*, int, int, int);

        bool open_stream();
        bool start_stream();
        bool stop_stream();
        bool close_stream();

        bool initialize_buffers(int count = 8);

        inline const std::string& get_error_msg() const { return _err_msg; }
        inline bool isvalid() const { return _isvalid; }
        inline int buffer_count() const { return _buffers.size(); }

        inline int get_fd() { return _fd; }

    private:
        bool set_framerate(int, float&);
        void stream();

        inline void set_error_msg(const std::string& msg)
        {
            _err_msg = msg;
            _isvalid = false;
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
