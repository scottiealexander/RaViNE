#include <fstream>

class FileSink
{
public:
    FileSink(const char* filepath) : _file(filepath, std::ofstream::binary)
    {
        
    }
    void process(void* data, size_t length)
    {
        
    }
private:
  std::ofstream _file;
  std::atomic_uint8_t _state;
};