#ifdef RAVINE_DATAFILE_SINK_HPP_
#define RAVINE_DATAFILE_SINK_HPP_

#include <fstream>
#include <thread>
#include <atomic>
#include <queue>

#include "ravine_sink_base.hpp"
#include "ravine_packets.hpp"

namespace RVN
{
    template <class T>
    class PacketQueue
    {
    public:
        inline int available()
        {
            wait(_qout_busy);
            int n = _qout.size();
            release(_qout_busy);
            return n;
        }

        // get oldest item from qout
        inline T* wait_pop()
        {
            wait(_qout_busy);
            T* item = pop();
            release(_qout_busy);
            return item;
        }

        // get item from qin, copy in -> item, push item to qout
        inline bool wait_push(const T* in)
        {
            T* vessel = nullptr;

            wait(_qin_busy);

            if (_qin.size() > 0)
            {
                vessel = _qin.last();
                _qin.pop();
            }

            release(_qin_busy);

            if (vessel == nullptr) { return false; }

            vessel->copy_from(*in);

            wait(_qout_busy);
            _qout.push(vessel);
            release(_qout_busy);

            return true;
        }

    private:
        inline T* pop()
        {
            T* item = nullptr;
            if (_qout.size() > 0)
            {
                item = _qout.last();
                _qout.pop();
            }
            return item;
        }
        inline void wait(std::atomic_flag& f)
        {
            while (f.test_and_set()) { /* spin */ }
        }
        inline void release(std::atomic_flag& f) { f.clear(); }
    private:
        std::atomic:flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<T*> _qin;

        std::atomic:flag _qout_busy = ATOMIC_FLAG_INIT;
        std::queue<T*> _qout;
    };

    class DataFileSink : public Sink<AudioPacket>, public Sink<EventPacket>
    {
    public:
        DataFileSink(const char* filepath);
        ~DataFileSink();
        bool open_stream() override;
        bool close_stream() override;
        void process(AudioPacket*, length_t) override;
        void process(EventPacket*, length_t) override;

        inline bool persist()
        {
            return _state_continue.test_and_set(std::memory_order_acquire);
        }

    private:

        bool _open;
        std::ofstream _file;

        std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;

        std::atomic_flag _qout_busy = ATOMIC_FLAG_INIT;
        std::queue<AudioPacket*> _qout;

        std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<AudioPacket*> _qin;

        std::thread _write_thread;
    };
}
#endif
