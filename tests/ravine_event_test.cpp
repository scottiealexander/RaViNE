//g++ -std=c++11 -DASIO_STANDALONE=1 -pedantic-errors -Wall -Wextra -pthread -I. -I/home/pi/Libraries/asio-1.12.2/include -o ./build/app/ravine_event_test ravine_event_source.cpp ravine_clock.cpp ravine_event_test.cpp
#include <iostream>
#include <cstdlib>

#include "ravine_utils.hpp"
#include "ravine_event_source.hpp"

int main(int narg, const char** args)
{
    int port = 65000;
    if (narg > 1)
    {
        port = std::atoi(args[1]);
        if (port < 1)
        {
            printf("[ERROR]: invalid port %s\n", args[1]);
            return -1;
        }
    }

    RVN::EventSource source(port);

    if (!source.open_stream())
    {
        std::cout << "[ERROR]: Failed to open stream" << std::endl;
        return -1;
    }

    (void)source.start_stream();

    while (!source.isready())
    {
        RVN::sleep_ms(100);
    }

    std::cout << "[INFO]: got connection, entering wait loop" << std::endl;

    // RVN::sleep_ms(1000);
    while (source.still_running())
    {
        RVN::sleep_ms(500);
    }

    if (!source.stop_stream())
    {
        std::cout << "[ERROR]: Failed to stop stream" << std::endl;
        (void)source.close_stream();
        return -1;
    }

    if (!source.close_stream())
    {
        std::cout << "[ERROR]: Failed to close stream" << std::endl;
        return -1;
    }

    return 0;
}
