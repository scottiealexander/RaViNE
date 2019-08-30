#include <fstream>
#include <sstream>
#include <iomanip>

#include <queue>
#include <cinttypes>
#include <cstring>

#include "ravine_buffer.hpp"

#define FILESINK_CONTINUE 0x00

// TODO
//  1. Make FrameBuffers re-usable... and don't free them in FileSink::write_loop
namespace ravine
{
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

        void process(void* data, size_t length, int width int height)
        {
            // NOTE TODO FIXME: get the next buffer from the q if there is one
            FrameBuffer* ptr = nullptr;
            if (_win.width == 0 || _win.height == 0)
            {
                ptr = new FullFrameBuffer(width, height);
            }
            else
            {
                ptr = new CroppedFrameBuffer(_win);
            }

            ptr->set_data(data, length, width);

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
        std::queue<Buffer*> _qout;

        std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<Buffer*> _qin;

    };
}
