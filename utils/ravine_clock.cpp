#include "ravine_clock.hpp"

namespace RVN
{
    const steady_clock::time_point Clock::_timebase = steady_clock::now();
}
