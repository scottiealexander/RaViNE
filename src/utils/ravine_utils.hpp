#ifndef RAVINE_UTILS_HPP_
#define RAVINE_UTILS_HPP_

#include <thread>
#include <chrono>
#include <atomic>
#include <queue>

namespace RVN
{
    /* ---------------------------------------------------------------------- */
    inline void sleep_ms(int n)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(n));
    }
    /* ---------------------------------------------------------------------- */
    inline bool wait_flag(std::atomic_flag& f)
    {
        return f.test_and_set(std::memory_order_acquire);
    }
    /* ---------------------------------------------------------------------- */
    inline void release_flag(std::atomic_flag& f) { f.clear(); }
    /* ---------------------------------------------------------------------- */
    template <typename T>
    T* pop_queue(std::queue<T*>& q)
    {
        T* ptr = q.front();
        q.pop();
        return ptr;
    }
    /* ---------------------------------------------------------------------- */
    template <typename T>
    void delete_queue(std::queue<T*> q)
    {
        for (int k = 0; k < q.size(); ++k)
        {
            T* tmp = pop_queue(q);
            if (tmp != nullptr)
            {
                delete tmp;
            }
        }
    }
    /* ---------------------------------------------------------------------- */
}
#endif
