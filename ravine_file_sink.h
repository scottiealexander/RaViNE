#include <fstream>
#include <sstream>
#include <iomanip>

#include <queue>
#include <cinttypes>
#include <cstring>

#define FILESINK_CONTINUE 0x00

struct CropWindow
{
    int col;
    int row;
    int width;
    int height;
}

class FrameBuffer
{
public:
    FrameBuffer(void* data, size_t length, int width) : _length(length/2), _width(width)
    {
        _height = static_cast<int>(_length / width);
        _data = new uint8_t[_length];
        
        size_t inc = 0;
        
        for (size_t k = 0; k < length; k += 2, ++inc)
        {
            if (inc < _length)
            {
                _data[inc] = data[k];
            }
        }
    }
    
    FrameBuffer(void* data, size_t length, int width, const CropWindow& crop) :
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
    
    ~FrameBuffer() { delete[] _data; }

    inline uint8_t* data() { return _data; }
    inline size_t length() const { return _length; }

private:
    uint8_t* _data;
    size_t _length;
    int _width;
    int _height;
};

class FileSink
{
public:
    FileSink(const char* filepath) : _frame(0), _state(0x00)
    {
        _win = {0,0,0,0};
    }
    FileSink(const char* filepath, CropWindow& win) : _frame(0), _win(win) {}
    ~FileSink()
    {
        for (int k = 0; k < _q.size(); ++k)
        {
            FrameBuffer* tmp = _q.front();
            _q.pop();
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
            ptr = new FrameBuffer(data, length);
        }
        else
        {
            ptr = new FrameBuffer(data, length, width, _win);
        }
        
        // wait for use of the queue
        while (_q_busy_flag.test_and_set()) {/*spin*/}
        _q.push(ptr);
        _q_busy_flag.clear();
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
        bool done = false;
        while (!done)
        {
            // wait until the next buffer is ready
            while (wait_buffer())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (!persist())
                {
                    done = true;
                    break;
                }
            }
            
            if (!done)
            {
                FrameBuffer* ptr = _q.front();
                _q.pop();
                _q_busy_flag.clear();
                
                if (next_file())
                {
                    _file << "P5" << std::endl << width << " " << height << std::endl << "255" << std::endl;
                    _file.write(buffer, width*height);
                    _file.close();
                }
            }
        }
    }
    inline bool wait_buffer() { return _q_busy_flag.test_and_set(std::memory_order_acquire); }
    inline bool persist() { return _state_continue.test_and_set(std::memory_order_acquire); }
private:
    int _frame;
    std::ofstream _file;
    
    std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;
    std::atomic_flag _q_busy_flag = ATOMIC_FLAG_INIT;
    
    CropWindow _win;
    std::queue<FrameBuffer*> _q;
    
};