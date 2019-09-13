//g++ -std=c++11 -pedantic-errors -Wall -Wextra -pthread -I. -I/home/scottie/Libraries/asio-1.12.2/include -o ./build/app/ravine_event_test ravine_event_source.cpp ravine_clock.cpp ravine_event_test.cpp
#include <iostream>

#include "ravine_utils.hpp"
#include "ravine_event_source.hpp"

#define port 65000

int main()
{
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

    RVN::sleep_ms(1000);

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
