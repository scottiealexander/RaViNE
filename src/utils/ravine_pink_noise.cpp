#include "ravine_pink_noise.hpp"

#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)

namespace RVN
{
    /* ====================================================================== */
    static unsigned long random_sample()
    {
        /* Change this seed for different random sequences. */
        static unsigned long seed = 22222;
        seed = (seed * 196314165) + 907633515;
        return seed;
    }
    /* ====================================================================== */
    PinkNoise::PinkNoise(int nrow, float noise_level) :
        _sum(0),
        _index(0),
        _index_mask((1<<nrow) - 1)
    {
        long pmax;

        /* Calculate maximum possible signed random value.
         * Extra 1 for white noise always added.
         * */
        pmax = (nrow + 1) * (1 << (PINK_RANDOM_BITS - 1));
        _scalar = noise_level / pmax;

        /* Initialize rows. */
        for(int k = 0; k < nrow; ++k) { _rows[k] = 0; }
    }
    /* ---------------------------------------------------------------------- */
    float PinkNoise::next_sample()
    {
        long new_sample;
        long sum;

        /* Increment and mask index. */
        int idx = next_index();

        /* If index is zero, don't update any random values. */
        if(idx != 0 )
        {
            /* Determine how many trailing zeros in _index. */
            /* This algorithm will hang if idx==0 so test first. */
            int nzero = 0;
            while((idx & 1) == 0)
            {
                idx = idx >> 1;
                ++nzero;
            }

            /* Replace the indexed ROWS random value.
             * Subtract and add back to _sum instead of adding all the random
             * values together. Only one changes each time.
             */
            _sum -= _rows[nzero];
            new_sample = ((long)random_sample()) >> PINK_RANDOM_SHIFT;
            _sum += new_sample;
            _rows[nzero] = new_sample;
        }

        /* Add extra white noise value. */
        new_sample = ((long)random_sample()) >> PINK_RANDOM_SHIFT;
        sum = _sum + new_sample;

        /* Scale to range of -1.0 to 0.9999. */
        return _scalar * (float)sum;
    }
    /* ====================================================================== */
}
