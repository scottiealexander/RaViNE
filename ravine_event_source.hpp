#ifndef RAVINE_EVENT_SOURCE_HPP_
#define RAVINE_EVENT_SOURCE_HPP_

#include <functional>
#include <thread>
#include "asio.hpp"

#include "ravine_clock.hpp"
#include "ravine_packets.hpp"
#include "ravine_sink_base.hpp"
#include "ravine_source_base.hpp"

namespace RVN
{
    /* ====================================================================== */
    typedef asio::ip::tcp tcp;
    /* ====================================================================== */
    class EventSource : public Source<EventPacket>
    {
    public:
        EventSource(int port) : _acceptor(_io, tcp::endpoint(tcp::v4(), port)),
            _socket(_io) {}

        bool open_stream() override;
        inline bool start_stream() override { return open_sink_stream(); }

        inline bool stop_stream() override
        {
            _socket.close();
            return close_sink_stream();
        }

        bool close_stream() override;

        inline bool isready() { return _socket.is_open(); }
        inline bool still_running() { return !_io.stopped(); }

    private:
        inline void async_read()
        {
            _socket.async_read_some(
                asio::buffer(&_buffer, 1),
                std::bind(&EventSource::handle_read, this,
                    std::placeholders::_1,
                    std::placeholders::_2
                )
            );
        }

        void handle_accept(const asio::error_code&);
        void handle_read(const asio::error_code&, uint32_t);

        void io_loop();

    private:
        asio::io_context _io;
        tcp::acceptor _acceptor;
        tcp::socket _socket;

        uint8_t _buffer;

        Clock _clock;
        std::thread _io_thread;
    };
    /* ====================================================================== */
}
#endif
