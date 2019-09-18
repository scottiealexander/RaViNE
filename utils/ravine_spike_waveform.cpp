#include <fstream>
#include "ravine_spike_waveform.hpp"

namespace RVN
{
    void WaveForm::load_waveform(const char* filepath)
    {
        std::ifstream ifs;
        ifs.open(filepath, std::ifstream::in | std::ifstream::binary);

        if (ifs.good())
        {
            ifs.read(reinterpret_cast<char*>(&_length), sizeof (_length));
            if (_length > 0)
            {
                _data = new float[_length];
                ifs.read(reinterpret_cast<char*>(_data), _length * sizeof (float));
                _owned = true;
                _ptr = 0;
            }
        }
        else
        {
            printf("[ERROR]: failed to open file %s\n", filepath);
        }
        ifs.close();
    }
}
