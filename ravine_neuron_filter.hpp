#ifndef RAVIE_NEURON_FILTER_HPP_
#define RAVIE_NEURON_FILTER_HPP_

#include <atomic>
#include <queue>

#include "ravine_clock.hpp"
#include "ravine_packets.hpp"
#include "ravine_filter_base.hpp"

namespace RVN
{
    class NeuronFilter : public Filter<YUYVImagePacket, BoolPacket>
    {
    public:
        NeuronFilter(const char* rf_file, int x, int y, int nbuf);
        ~NeuronFilter();
        bool open_stream() override;
        bool close_stream() override;
        bool start_stream() override;
        bool stop_stream() override;

        inline bool is_open() { return _open; }

        inline void process(YUYVImagePacket* packet, length_t bytes);

        inline int width() const { return _win.width; }
        inline int height() const { return _win.height; }

        inline const std::string& get_error_msg() const { return _err_msg; }

        inline bool isvalid() const { return _isvalid; }

    private:
        bool read_rf_file(const char*, int&, int&);
        void allocate_buffers(int n);

        void filter(YUYVImagePacket* img, length_t bytes, float& act);

        void forward_loop();

        inline bool persist()
        {
            return _state_continue.test_and_set(std::memory_order_acquire);
        }

        inline void set_error_msg(const std::string& msg)
        {
            if (isvalid())
            {
                _err_msg = msg;
                _isvalid = false;
            }
            else
            {
                _err_msg.append(" " + msg);
            }
        }

    private:

        bool _open;

        bool _isvalid;
        std::string _err_msg;

        CropWindow _win;
        uint8_t* _rf = nullptr;

        float _rf_mag;
        float _rf_mean;

        std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;

        std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        std::queue<FloatPacket*> _qin;

        std::atomic_flag _qout_busy = ATOMIC_FLAG_INIT;
        std::queue<FloatPacket*> _qout;

        std::thread _process_thread;

        Clock _clock;

        static constexpr float _threshold = 0.2f;

    };
}

#endif
