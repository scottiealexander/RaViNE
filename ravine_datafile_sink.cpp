#include <cstdio>

#include "ravine_utils.hpp"
#include "ravine_datafile_sink.hpp"

namespace RVN
{
    /* ---------------------------------------------------------------------- */
    DataFileSink::DataFileSink(const char* filepath, int frames_per_buffer) :
        _audio_stream(8), _filepath(filepath)
    {
        // on init, we do not have any events, set no_event flag to true
        // as false indicates the presence of an event
        _no_event.test_and_set(std::memory_order_acquire);

        if (_audio_stream.isvalid())
        {
            printf("[INFO]: audio stream appears valid\n");

            // construct a temporary buffer that will be cloned to fill the
            // buffers in the DataConveyor _audio_stream
            AudioBuffer temp(frames_per_buffer);
            temp.fill(0.0f);

            printf("[INFO]: cloning buffer of size %d, should be %d\n", temp.length(), frames_per_buffer);

            if (!_audio_stream.fill(temp))
            {
                set_error_msg("Failed during audio stream fill");
            }
            else
            {
                printf("[INFO]: audio stream filled from clone\n");
            }
        }
    }
    /* ---------------------------------------------------------------------- */
    DataFileSink::~DataFileSink()
    {
        printf("[INFO]: ~DataFileSink\n");
    }
    /* ---------------------------------------------------------------------- */
    bool DataFileSink::open_stream()
    {
        if (isvalid() && !isopen())
        {
            // indicate that we should continue streaming to file...
            printf("[INFO]: setting persist flag to true\n");
            _state_continue.test_and_set();

            printf("[INFO]: Starting write thread!\n");
            _write_thread = std::thread(&DataFileSink::write_loop, this);
            this->_isopen = true;
        }
        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    bool DataFileSink::close_stream()
    {
        printf("[INFO]: Joining write thread\n");
        if (isopen())
        {
            _state_continue.clear();
            _write_thread.join();
            this->_isopen = false;
        }
        return isvalid();
    }
    /* ---------------------------------------------------------------------- */
    void DataFileSink::process(AudioPacket* packet, length_t /* bytes */)
    {
        // make sure we are still accepting packets
        if (persist())
        {
            // if we have a packet that is ready to be loaded, load it, otherwise
            // drop the frame?
            if (_audio_stream.load_ready())
            {
                AudioBuffer* buf = _audio_stream.pop_load();
                buf->copy(packet);
                _audio_stream.push_load(buf);
            }
            else
            {
                printf("[ERROR]: dropped audio packet...\n");
            }
        }
        else
        {
            // leave continue flag in the false state, drop the packet
            _state_continue.clear();
        }
    }
    /* ---------------------------------------------------------------------- */
    void DataFileSink::process(EventPacket* packet, length_t /* bytes */)
    {
        // make sure we are still accepting packets
        if (persist())
        {
            // events are by definition (in this sytem) "rare", so have_event()
            // should, in theory, never be true when this gets called, but we'll
            // check just to be safe
            if (!have_event())
            {
                // copy the packet and indicate that we have an event
                _event_packet = packet;
                _no_event.clear();
            }
            else
            {
                // NOTE: this is not thread safe! (but should also never happen
                // in practice)
                printf("[ERROR]: dropped event packet...\n");
            }
        }
        else
        {
            // leave continue flag in the false state, drop the packet
            _state_continue.clear();
        }
    }
    /* ---------------------------------------------------------------------- */
    int32_t DataFileSink::write_header()
    {
        int32_t offset = -1;
        if (_file.is_open())
        {
            // channel types:
            //    0x02 -> 0010 -> uint8
            //    0x04 -> 0100 -> uint16
            //    0x06 -> 0110 -> uint32
            //    0x03 -> 0011 -> int8
            //    0x05 -> 0101 -> int16
            //    0x07 -> 0111 -> int32
            //    0x0f -> 1111 -> float32

            const uint8_t hdr[9] =
                {
                    0x02, // 2 channels
                    0x01, // id of channel 1 (audio channel)
                    0x02, // id of channel 2 (event channel)
                    0x0f, // d-type of channel 1 (float)
                    0x01, // d-type of channel 2 (uint8_t)
                    0x00, 0x00, 0x00, 0x00 // place holder for int32 packet count
                };

            _file.write(reinterpret_cast<const char*>(hdr), sizeof (hdr));
            offset = sizeof (hdr) - sizeof (int32_t);
        }

        return offset;
    }
    /* ---------------------------------------------------------------------- */
    void DataFileSink::process_audio_queue(int32_t& count)
    {
        static const uint8_t id = 0x01;

        while (_audio_stream.unload_ready())
        {
            AudioBuffer* buf = _audio_stream.unload();

            int32_t len = buf->length();
            float time = buf->timestamp();

            // a packet: {id::uint8, time::float, length::int32, data::array}
            // where the type of data is given by the entry in the channel
            // type array in the header that corresponds to the given id
            _file.write(reinterpret_cast<const char*>(&id), sizeof (id));
            _file.write(reinterpret_cast<char*>(&time), sizeof (time));
            _file.write(reinterpret_cast<char*>(&len), sizeof (len));
            _file.write(reinterpret_cast<char*>(buf->data()), len * sizeof (float));

            // buffer goes back in the cycle to be reloaded with data
            _audio_stream.reload(buf);

            ++count;
        }
    }
    /* ---------------------------------------------------------------------- */
    void DataFileSink::process_event_queue(int32_t& count)
    {
        static const uint8_t id = 0x02;
        static const int32_t len = 1;

        if (have_event())
        {
            const float time = _event_packet.timestamp();

            _file.write(reinterpret_cast<const char*>(&id), sizeof (id));
            _file.write(reinterpret_cast<const char*>(&time), sizeof (time));
            _file.write(reinterpret_cast<const char*>(&len), sizeof (len));

            _file.write(
                reinterpret_cast<const char*>(_event_packet.data()),
                sizeof (uint8_t)
            );

            // set no_event back to true (releases _event_packet and indicates
            // readiness to accept another event)
            _no_event.test_and_set(std::memory_order_acquire);

            ++count;
        }
    }
    /* ---------------------------------------------------------------------- */
    void DataFileSink::write_loop()
    {
        _file.open(_filepath, std::ofstream::out | std::ofstream::binary);
        if (!_file.is_open())
        {
            set_error_msg("Failed to open file");
            return;
        }

        // offset (in bytes) in the file where we should write the packet count
        // when all packets have been received
        int32_t count_offset = write_header();

        if (count_offset < 1)
        {
            set_error_msg("Failed to write file header");
            _file.close();
            return;
        }

        int32_t packet_count = 0;

        while (persist())
        {
            process_audio_queue(packet_count);
            process_event_queue(packet_count);

            // give some time to other processes
            sleep_ms(10);
        }

        // make sure to write any buffers that remain in the output queue to
        // the file
        process_audio_queue(packet_count);
        process_event_queue(packet_count);

        // seek to the location where we should insert the packet count
        _file.seekp(count_offset, _file.beg);
        _file.write(reinterpret_cast<char*>(&packet_count), sizeof (packet_count));

        _file.close();

        printf("[STATS]: total packets: %d\n", packet_count);
    }
    /* ---------------------------------------------------------------------- */
}
