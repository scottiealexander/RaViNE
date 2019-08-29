#include <fstream>
#include <sstream>
#include <iomanip>

#include <queue>
#include <cinttypes>
#include <cstring>

#define FILESINK_CONTINUE 0x00

// TODO
//  1. Make FrameBuffers re-usable... and don't free them in FileSink::write_loop

struct CropWindow
{
    int col;
    int row;
    int width;
    int height;
    inline size_t length() const
    {
        return static_cast<size_t>(width) * static_cast<size_t>(height);
    }
}

class FrameBuffer
{
public:
    FrameBuffer(CropWindow* win) : _win(win), _length(win->length())
    {
        _data = new uint8_t[_length];
    }

    ~FrameBuffer()
    {
        if (_data != nullptr)
        {
            delete[] _data;
        }
    }

    void set_data(void* data, size_t length, int width) : _length(length/2), _width(width)
    {
        size_t inc = 0;

        for (size_t k = 0; k < length; k += 2, ++inc)
        {
            if (inc < _length)
            {
                _data[inc] = data[k];
            }
        }
    }

    void set_data(void* data, size_t length, int width, const CropWindow& crop) :
        _length(crop.width*crop.height), _width(crop.width), _height(crop.height)
    {
        _data = new uint8_t[_length];
        int inc = 0;
        int w2 = width * 2;

        for (int k = crop.row; k < crop.height; ++k)
        {
            for (int j = (crop.col * 2); j < (crop.width*2); j+=2)
            {
                int idx = (k * w2 + j);
                if (idx < length)
                {
                    _data[inc] = data[idx];
                    ++inc;
                }
            }
        }
    }

    inline uint8_t* data() { return _data; }
    inline size_t length() const { return _length; }

private:
    uint8_t* _data = nullptr;
    size_t _length;
    CropWindow* _win;
    int _width;
    int _height;
};

class FileSink
{
public:
    FileSink(const char* filepath) : _frame(0)
    {
        _win = {0,0,0,0};
    }

    FileSink(const char* filepath, CropWindow& win) : _frame(0), _win(win) {}

    ~FileSink()
    {
        for (int k = 0; k < _qout.size(); ++k)
        {
            FrameBuffer* tmp = _qout.front();
            _qout.pop();
            if (tmp != nullptr)
            {
                delete tmp;
            }
        }
    }

    void process(void* data, size_t length, int width)
    {
        FrameBuffer* ptr = nullptr;
        if (_win.width == 0 || _win.height == 0)
        {
            ptr = new FrameBuffer(data, length, width);
        }
        else
        {
            ptr = new FrameBuffer(data, length, width, _win);
        }

        // wait for use of the queue, this function needs to return asap
        // so as not to block the frame acqusition thread, so no sleep
        while (wait_queue()) {/*spin until queue is available*/}
        _qout.push(ptr);
        release_queue();
    }

private:
    bool next_file()
    {
        std::ostringstream os;
        os << "./frames/frame-" << std::setfill('0') << std::setw(3) << _frame << ".pgm";
        _file.open(os.str(), std::ofstream::binary);
        ++_frame;
        return _file.is_open();
    }

    void write_loop()
    {
        // write frames as they become available unti we receive the terminate signal
        while (persist())
        {
            // wait until the queue is available (does not imply that there is a new buffer)
            while (wait_queue())
            {
                // sleeping here has little cost, so allow OS to do other things
                // (like acquire frames...)
                sleep_ms(1);
            }

            // if a new buffer is available, write it to file
            if (_qout.size() > 0)
            {
                FrameBuffer* ptr = pop_queue();
                release_queue();

                if (next_file())
                {
                    _file << "P5" << std::endl << width << " " << height << std::endl << "255" << std::endl;
                    _file.write(buffer, width*height);
                    _file.close();
                }

                // NOTE TODO FIXME free the FrameBuffer or re-use it?
                while (wait_queue_in()) { sleep_ms(1); }
                _qin.push(ptr);
                release_queue_in();
            }
            else
            {
                release_queue();

                // queue is empty, might as well wait
                sleep_ms(10);
            }

        }
    }

    inline bool wait_queue() { return _qout_busy.test_and_set(std::memory_order_acquire); }
    inline bool wait_queue_in() { return _qin_busy.test_and_set(std::memory_order_acquire); }
    inline void release_queue() { _qout_busy.clear(); }
    inline void release_queue_in() { _qin_busy.clear(); }

    inline Framebuffer* pop_queue()
    {
        FrameBuffer* ptr = _qout.front();
        _qout.pop();
        return ptr;
    }

    inline bool persist() { return _state_continue.test_and_set(std::memory_order_acquire); }

    inline void sleep_ms(int n) { std::this_thread::sleep_for(std::chrono::milliseconds(n)); }

private:
    int _frame;
    std::ofstream _file;

    std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;

    CropWindow _win;

    std::atomic_flag _qout_busy = ATOMIC_FLAG_INIT;
    std::queue<FrameBuffer*> _qout;

    std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
    std::queue<FrameBuffer*> _qin;

};
