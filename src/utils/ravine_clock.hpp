#ifndef RAVINE_CLOCK_HPP_
#define RAVINE_CLOCK_HPP_

#include <chrono>

namespace RVN
{
    /* ---------------------------------------------------------------------- */
    typedef std::chrono::steady_clock steady_clock;

    class Clock
    {
    public:
        Clock() {}
        inline float now() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                steady_clock::now() - _timebase
            ).count() * usec2sec;
        }

    public:
        static const steady_clock::time_point _timebase;
        static constexpr float usec2sec = 1e-6f;
    };
    /* ---------------------------------------------------------------------- */
}
#endif
