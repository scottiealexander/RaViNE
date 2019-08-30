#ifndef RAVINE_UTILS_HPP_
#define RAVINE_UTILS_HPP_

#include <thread>
#include <chrono>

inline void sleep_ms(int n)
{ std::this_thread::sleep_for(std::chrono::milliseconds(n)); }

#endif
