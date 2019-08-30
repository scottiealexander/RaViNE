#ifndef RAVINE_FILE_SINK_HPP_
#define RAVINE_FILE_SINK_HPP_

#include <fstream>
#include <queue>

#include <cinttypes>

#include "ravine_buffer.hpp"

namespace ravine
{
    class FileSink
    {
    public:
        FileSink(const char* filepath, int width, int height, int nbuffer);
        FileSink(const char* filepath, CropWindow& win, int nbuffer);
        ~FileSink();

        bool process(void* data, uint32_t length, int width);

    private:
        void allocate_buffers(int n);
        bool next_file();
        void write_loop();

        inline bool persist()
        { return _state_continue.test_and_set(std::memory_order_acquire); }

        inline void sleep_ms(int n)
        { std::this_thread::sleep_for(std::chrono::milliseconds(n)); }

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
}
#endif
