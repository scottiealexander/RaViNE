#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <queue>

#include <cinttypes>

#include "ravine_utils.hpp"
#include "ravine_frame_buffer.hpp"
#include "ravine_file_sink.hpp"

namespace RVN
{
    /* ====================================================================== */
    inline FrameBuffer* pop_queue(std::queue<FrameBuffer*>& q)
    {
        FrameBuffer* ptr = q.front();
        q.pop();
        return ptr;
    }
    /* ---------------------------------------------------------------------- */
    inline bool wait_flag(std::atomic_flag& f)
    {
        return f.test_and_set(std::memory_order_acquire);
    }
    /* ---------------------------------------------------------------------- */
    inline void release_flag(std::atomic_flag& f) { f.clear(); }
    /* ---------------------------------------------------------------------- */
    void delete_queue(std::queue<FrameBuffer*> q)
    {
        for (int k = 0; k < q.size(); ++k)
        {
            FrameBuffer* tmp = pop_queue(q);
            if (tmp != nullptr)
            {
                // proper deconstructor is the base class one
                delete tmp;
            }
        }
    }
    /* ====================================================================== */
    FileSink::FileSink(const CropWindow& win, int nbuff) : _win(win)
    {
        init(nbuff);
    }
    /* ---------------------------------------------------------------------- */
    FileSink::FileSink(int width, int height, int nbuff)
    {
        _win = {0, 0, width, height};
        init(nbuff);
    }
    /* ---------------------------------------------------------------------- */
    FileSink::~FileSink()
    {
        delete_queue(_qout);
        delete_queue(_qin);
        if (_file.is_open())
        {
            _file.close();
        }
    }
    /* ---------------------------------------------------------------------- */
    void FileSink::init(int nbuff)
    {
        _state_continue.test_and_set();
        allocate_buffers(nbuff);
        _open = false;
        _frame = 0;
    }
    /* ---------------------------------------------------------------------- */
    bool FileSink::open_stream()
    {
        if (_qin.size() < 1) { return false; }

        if (!is_open())
        {
            _write_thread = std::thread(&FileSink::write_loop, this);
            _open = true;
        }
        return true;
    }
    /* ---------------------------------------------------------------------- */
    bool FileSink::close_stream()
    {
        if (is_open())
        {
            // set "continue" flag to false
            _state_continue.clear();
            _write_thread.join();
            _open = false;
        }
        return true;
    }
    /* ---------------------------------------------------------------------- */
    void FileSink::process(YUYVImagePacket* packet)
    {
        if (is_open())
        {
            // wait for use of the queue, this function needs to return asap
            // so as not to block the frame acqusition thread, so no sleep
            while (wait_flag(_qin_busy)) {/* spin until queue is available */}

            // no buffers are available, drop the frame...
            if (_qin.size() < 1) { return; }

            FrameBuffer* ptr = pop_queue(_qin);
            release_flag(_qin_busy);

            // copy data, no alloc / free
            ptr->set_data(packet);

            // again, no sleep to stay quick
            while (wait_flag(_qout_busy)) {/* spin */}
            _qout.push(ptr);
            release_flag(_qout_busy);
        }
    }
    /* ---------------------------------------------------------------------- */
    void FileSink::allocate_buffers(int n)
    {
        bool full = _win.col == 0 && _win.row == 0;

        for (int k = 0; k < n; ++k)
        {
            FrameBuffer* ptr;
            if (full)
            {
                ptr = new FullFrameBuffer(_win.width, _win.height);
            }
            else
            {
                ptr = new CroppedFrameBuffer(&_win);
            }

            // this should only be called from a constructor so no need
            // with wait on _qin_busy
            _qin.push(ptr);
        }
    }
    /* ---------------------------------------------------------------------- */
    bool FileSink::next_file()
    {
        std::ostringstream os;
        os << "./frames/frame-" << std::setfill('0') << std::setw(3) << _frame << ".pgm";
        _file.open(os.str(), std::ofstream::binary);
        ++_frame;
        return _file.is_open();
    }
    /* ---------------------------------------------------------------------- */
    void FileSink::write_loop()
    {
        // write frames as they become available unti we receive the terminate signal
        while (persist())
        {
            // wait until the queue is available (does not imply that there is a new buffer)
            while (wait_flag(_qout_busy))
            {
                // sleeping here has little cost, so allow OS to do other things
                // (like acquire frames...)
                sleep_ms(1);
            }

            // if a new buffer is available, write it to file
            if (_qout.size() > 0)
            {
                FrameBuffer* ptr = pop_queue(_qout);
                release_flag(_qout_busy);

                if (next_file())
                {
                    _file << "P5" << std::endl << ptr->width() << " " << ptr->height() << std::endl << "255" << std::endl;
                    _file.write(reinterpret_cast<char*>(ptr->data()), ptr->length());
                    _file.close();
                }

                // reuse the FrameBuffer once _qin is free
                while (wait_flag(_qin_busy)) { sleep_ms(1); }
                _qin.push(ptr);
                release_flag(_qin_busy);
            }
            else
            {
                // no buffer ready for writing, release _qout
                release_flag(_qout_busy);

                // queue is empty, might as well actually wait
                sleep_ms(10);
            }

        }
    }
    /* ====================================================================== */
}
