#ifndef RAVINE_FILE_SINK_HPP_
#define RAVINE_FILE_SINK_HPP_

#include <fstream>
#include <thread>
#include <atomic>
#include <queue>

#include <cinttypes>

#include "ravine_frame_buffer.hpp"
#include "ravine_packets.hpp"
#include "ravine_sink_base.hpp"

namespace RVN
{
    class FileSink : public Sink<YUYVImagePacket>
    {
    public:
        FileSink(const CropWindow& win, int nbuf);
        FileSink(int width, int height, int nbuf);
        ~FileSink();

        bool open_stream() override;
        bool close_stream() override;
        void process(YUYVImagePacket* packet) override;

        inline bool is_open() { return _open; }

    private:
        void init(int n);
        void allocate_buffers(int n);
        bool next_file();
        void write_loop();

        inline bool persist()
        {
            return _state_continue.test_and_set(std::memory_order_acquire);
        }

    private:
        int _frame;
        bool _open;
        std::ofstream _file;

        std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;

        CropWindow _win;

        std::atomic_flag _qout_busy = ATOMIC_FLAG_INIT;
        std::queue<FrameBuffer*> _qout;

        std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<FrameBuffer*> _qin;

        std::thread _write_thread;
    };
}
#endif
