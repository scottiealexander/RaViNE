#ifndef RAVIE_NEURON_FILTER_HPP_
#define RAVIE_NEURON_FILTER_HPP_

#include <atomic>
#include <queue>

#include "ravine_packets.hpp"
#include "ravine_filter_base.hpp"
#include "ravine_frame_buffer.hpp"

namespace RVN
{
    class FloatPacket : public Packet<float>
    {
    public:
        inline float& get_data() { return _data; }
        inline void set_data(float x) { this->_data = x; }
    }

    class NeuronFilter : public Filter<YUYVImagePacket, FloatPacket>
    {
    public:
        NeuronFilter(const CropWindow& win) : _win(win) {}
        ~NeuronFilter();
        bool open_stream() override;
        bool close_stream() override;
        bool start_stream() override;
        bool stop_stream() override;

        inline bool is_open() { return _open; }

        inline void process(YUYVImagePacket* packet, length_t bytes);

    private:
        void allocate_buffers(int n);

        void filter(YUYVImagePacket* img, length_t bytes, float& act);

        inline bool persist()
        {
            return _state_continue.test_and_set(std::memory_order_acquire);
        }

    private:

        bool _open;
        std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;

        CropWindow _win;

        std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<FloatPacket*> _qin;

        std::atomic_flag _qout_busy = ATOMIC_FLAG_INIT;
        std::queue<FloatPacket*> _qout;

        std::thread _process_thread;

    };
}

#endif