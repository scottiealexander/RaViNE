#ifndef RAVINE_VIDEO_UTILS_HPP_
#define RAVINE_VIDEO_UTILS_HPP_

#include <string>
#include <vector>
#include <cstring>

#include <atomic>
#include <thread>

#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

/* ===========================================================================*/
struct MMBuffer
{
    MMBuffer(const int, const size_t, const size_t);
    ~MMBuffer();

    void* start;
    size_t length;
    bool valid;
};

using BufferVec = std::vector<MMBuffer>;
/* ===========================================================================*/
class V4L2
{
public:
    V4L2();
    ~V4L2();
    bool open_stream(const char*, const int, const int, const int);
    bool start_stream();
    bool stop_stream();
    bool close_stream();

    bool set_framerate(const int&, float&);
    bool init_stream(const int count = 8);

    inline const std::string& get_error_msg() const { return _err_msg; }
    inline bool isvalid() const { return _isvalid; }
    inline int buffer_count() const { return _buffers.size(); }

private:
    void stream();

    inline void set_error_msg(const std::string& msg)
    {
        _err_msg = msg;
        _isvalid = false;
    }

    inline bool persist()
    { return _state_continue.test_and_set(std::memory_order_acquire); }
    inline void send_stop() { _state_continue.clear(); }

private:
    int _fd;
    bool _isvalid;

    int _width;
    int _height;

    BufferVec _buffers;

    std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;
    std::thread _stream_thread;

    std::string _err_msg;
};
/* ===========================================================================*/
#endif
