#ifndef RAVINE_PINK_NOISE_HPP_
#define RAVINE_PINK_NOISE_HPP_

#define PINK_MAX_RANDOM_ROWS   (30)

namespace RVN
{
    class PinkNoise
    {
    public:
        PinkNoise(int nrow);
        float next_sample();

        inline int next_index()
        {
            _index = (_index + 1) & _index_mask;
            return _index;
        }

    private:
        long      _rows[PINK_MAX_RANDOM_ROWS];
        long      _sum;          /* Used to optimize summing of generators. */
        int       _index;        /* Incremented each sample. */
        int       _index_mask;   /* Index wrapped by ANDing with this mask. */
        float     _scalar;       /* Used to scale within range of -1.0 to +1.0 */
    };

    static unsigned long random_sample();

}
#endif
