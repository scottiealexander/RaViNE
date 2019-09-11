#ifdef RAVINE_DATAFILE_SINK_HPP_
#define RAVINE_DATAFILE_SINK_HPP_

#include <fstream>
#include <thread>
#include <atomic>
//#include <queue>
#include <algorithm>

#include <cstdio>
#include <cstring>

#include "ravine_sink_base.hpp"
#include "ravine_packets.hpp"
#include "ravine_data_conveyor.hpp"

namespace RVN
{
    /* ====================================================================== */
    // this is just an AudioPacket that manages it's own memory and can do
    // copy construct / copy fill
    class AudioBuffer : public AudioPacket
    {
    public:
        AudioBuffer(int length) :
            AudioPacket(new float[length], length, -1.0f) {}

        AudioBuffer(const AudioBuffer& other) :
            AudioPacket(new float[other.length()], other.length(), other.timestamp())
        {
            memcpy(this->_data, other.data(), other.length() * sizeof (float));
        }

        ~AudioBuffer()
        {
            if (this->_data != nullptr)
            {
                printf("[INFO]: deleting an audio buffer\n");
                delete[] this->_data;
            }
        }

        inline void fill(float d) { std::fill_n(this->_data, this->length(), d); }

        // this is basically a operator=() overload for copying a packet
        inline void copy(AudioPacket* packet)
        {
            // make sure we call AudioPacket's length() to get the actual length
            // of the underlying float array
            length_t len = RVN_MIN(packet->length(), AudioPacket::length());

            memcpy(this->_data, packet->data(), len * sizeof (float));

            // our temporary length for this transfer, in practice this should
            // *ALWAYS* be the the original length (packer->length() ==
            // this->length()), but just to be safe we keep the smaller of the
            // two as our "working length"
            this->_working_length = len;

            this->_time = packet->timestamp;
        }

        inline length_t length() const override { return _working_length; }

    private:
        length_t _working_length;
    }
    /* ====================================================================== */
    class DataFileSink : public Sink<AudioPacket>//, public Sink<EventPacket>
    {
    public:
        DataFileSink(const char*, int);
        ~DataFileSink();
        bool open_stream() override;
        bool close_stream() override;
        void process(AudioPacket*, length_t) override;
        //void process(EventPacket*, length_t) override;

        inline bool persist()
        {
            return _state_continue.test_and_set(std::memory_order_acquire);
        }

        inline bool isopen() const { return _stream_open; }

        inline bool isvalid() const { return !_error; }
        inline const std::string& get_error_msg() const { return _err_msg; }

    private:
        void set_error_msg(const char* msg)
        {
            _err_msg = msg;
            _error = true;
        }

        int32_t write_header();
        void write_chunk(int32_t&);
        void write_loop();

    private:
        DataConveyor<AudioBuffer> _audio_stream;

        bool _error;
        std::string _error_msg;

        bool _stream_open = false;
        const std::string _filepath;
        std::ofstream _file;

        std::atomic_flag _state_continue = ATOMIC_FLAG_INIT;
        std::thread _write_thread;

        //std::atomic_flag _qout_busy = ATOMIC_FLAG_INIT;
        //std::queue<EventPacket*> _qout;

        //std::atomic_flag _qin_busy = ATOMIC_FLAG_INIT;
        //std::queue<EventPacket*> _qin;
    };
    /* ====================================================================== */
}
#endif
