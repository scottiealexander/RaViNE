#ifndef RAVINE_SPIKE_WAVEFORM_HPP_
#define RAVINE_SPIKE_WAVEFORM_HPP_

#include <cinttypes>

namespace RVN
{
    /* ====================================================================== */
    class WaveForm
    {
    public:
        WaveForm() : _length(0), _owned(false) {};

        WaveForm(const char* file) : _ptr(0), _owned(true)
        {
            load_waveform(file);
        }

        WaveForm(float* data, int length) :
            _data(data), _length(length), _ptr(0), _owned(false) {}

        ~WaveForm()
        {
            if (_owned && (_data != nullptr))
            {
                delete[] _data;
            }
        }

        void load_waveform(const char* filepath);

        inline float next_sample(bool& still_spiking)
        {
            float out = _data[_ptr];
            _ptr = (_ptr + 1) % _length;

            // return true until the full waveform has been returned
            still_spiking = _ptr > 0;

            return out;
        }

        inline void reset() { _ptr = 0; }
        inline int loc() { return _ptr; }

        inline bool isvalid() { return (_data != nullptr) && (_length > 0); }

    private:
        float* _data = nullptr;
        int32_t _length;
        int _ptr;

        bool _owned;
    };
    /* ====================================================================== */
    inline WaveForm default_waveform() { return WaveForm("./spike.wf"); }
    /* ====================================================================== */
}
#endif
