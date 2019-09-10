#ifdef RAVINE_DATAFILE_SINK_HPP_
#define RAVINE_DATAFILE_SINK_HPP_

#include <fstream>
#include <thread>
#include <atomic>
#include <queue>

#include "ravine_sink_base.hpp"
#include "ravine_packets.hpp"

#include "pa_ringbuffer.h"
#include "pa_util.h"

namespace RVN
{
    template <class T>
    class RingBuffer
    {
    public:
        RingBuffer(int length) : _isvalid(false), _length(legnth)
        {
            _data = PaUtil_AllocateMemory(sizeof(T*) * _length);
            if (_data != nullptr)
            {
                PaUtil_InitializeRingBuffer(&_buffer, sizeof(T*), _length, _data);
                _isvalid = true;
            }
        }
        ~RingBuffer()
        {
            if (_data != nullptr)
            {
                PaUtil_FreeMemory(_data);
            }
        }
    private:
        bool _isvalid;
        int _length;
        PaUtilRingBuffer _buffer;
        void* _data = nullptr;
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
        std::queue<EventPacket*> _qout;

        std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<EventPacket*> _qin;

        std::thread _write_thread;
    };
}
#endif
