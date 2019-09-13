#include <iostream>
#include "ravine_event_source.hpp"

namespace RVN
{
    /* ====================================================================== */
    bool EventSource::open_stream()
    {
        _acceptor.async_accept(
            _socket,
            std::bind(&EventSource::handle_accept, this, std::placeholders::_1)
        );

        // launch our io_context thread: this just call io_context::run() in
        // a seperate thread to allow control to return to the caller
        _io_thread = std::thread(&EventSource::io_loop, this);

        return true;
    }
    /* ---------------------------------------------------------------------- */
    bool EventSource::close_stream()
    {
        if (_socket.is_open()) { _socket.close(); }

        // signal to the io_context that it should end
        _io.stop();

        // wait until it actually does
        _io_thread.join();

        return true;
    }
    /* ---------------------------------------------------------------------- */
    void EventSource::handle_accept(const asio::error_code& ec)
    {
        if (!ec)
        {
            async_read();
        }
    }
    /* ---------------------------------------------------------------------- */
    void EventSource::handle_read(const asio::error_code& ec, uint32_t bytes)
    {
        const float time = _clock.now();

        if (!ec)
        {
            if (bytes > 0)
            {
                //EventPacket packet(_buffer, time);
                //send_sink(&packet, 1);
                std::cout << "[REC]: " << (int)_buffer << " @ " << time << std::endl;
            }

            // if we get the close signal (255) we do *NOT* start another read
            // operation, which will allow our io_context::run() (in io_loop())
            // to return as it will have no tasks left to run
            if (_buffer != 0xff)
            {
                async_read();
            }
        }
    }
    /* ---------------------------------------------------------------------- */
    void EventSource::io_loop()
    {
        try
        {
            // io_context::run is blocking, so we start it in a seperate thread
            // so as not to block the main one, run() is what allows accepting
            // and other async io commands to function
            _io.run();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
    /* ====================================================================== */
}
