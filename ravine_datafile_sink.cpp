#include <cstdio>

#include "ravine_utils.hpp"
#include "ravine_datafile_sink.hpp"

namespace RVN
{
    /* ---------------------------------------------------------------------- */
    DataFileSink::DataFileSink(const char* filepath, int frames_per_buffer) :
        _audio_stream(16), _error(false), _filepath(filepath)
    {
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
        // if we have a packet that is ready to be loaded, load it, otherwise
        // drop the frame?
        if (_audio_stream.load_ready())
        {
            AudioBuffer* buf = _audio_stream.pop_load();
            buf->copy(packet);
            _audio_stream.push_load(buf);
        }
        //else
        //{
            //printf("[ERROR]: dropped audio packet...\n");
        //}
    }
    /* ---------------------------------------------------------------------- */
    //void DataFileSink::process(EventPacket* packet, length_t bytes)
    //{

    //}
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

            // 1 channel,  ids = [0x01], types = [0x0f], packet count (int32)
            const uint8_t hdr[7] = {0x01, 0x01, 0x0f, 0x00, 0x00, 0x00, 0x00};
            _file.write(reinterpret_cast<const char*>(hdr), 7);
            offset = 3;
        }
        return offset;
    }
    /* ---------------------------------------------------------------------- */
    void DataFileSink::write_chunk(int32_t& count, int32_t& zero)
    {
        static const uint8_t id = 0x01;

        while (_audio_stream.unload_ready())
        {
            AudioBuffer* buf = _audio_stream.unload();

            int n = 0;
            for (int k = 0; k < buf->length(); ++k)
            {
                if (buf->data()[k] == 0.0f) { ++n; }
            }

            if (n == buf->length())
            {
                ++zero;
                printf("[WARNING]: zero buffer @ %.10f\n", buf->timestamp());
            }

            int32_t len = buf->length();
            float time = buf->timestamp();

            // a packet: {id::uint8, time::float, length::int32, data::array}
            // where the type of data is given by the entry in the channel
            // type array in the header that corresponds to the given id
            _file.write(reinterpret_cast<const char*>(&id), sizeof (id));
            _file.write(reinterpret_cast<char*>(&time), sizeof (time));
            _file.write(reinterpret_cast<char*>(&len), sizeof (len));
            _file.write(reinterpret_cast<char*>(buf->data()), len * sizeof (float));


            buf->mark_empty();

            // buffer goes back in the cycle to be reloaded with data
            _audio_stream.reload(buf);

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
        int32_t zero_count = 0;

        while (persist())
        {
            write_chunk(packet_count, zero_count);

            // give some time to other processes
            sleep_ms(10);
        }

        // make sure to write any buffers that remain in the output queue to
        // the file
        write_chunk(packet_count, zero_count);

        // seek to the location where we should insert the packet count
        _file.seekp(count_offset, _file.beg);
        _file.write(reinterpret_cast<char*>(&packet_count), sizeof (packet_count));

        _file.close();

        printf("[STATS]: total packets: %d, zero packets: %d\n", packet_count, zero_count);
    }
    /* ---------------------------------------------------------------------- */
}
