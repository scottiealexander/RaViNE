#define min(x, y) ((x) < (y) ? (x) : (y))

namespace ravine
{
    struct CropWindow
    {
        int col;
        int row;
        int width;
        int height;
        inline uint32_t length() const
        {
            return static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
        }
    }

    class FrameBuffer
    {
    public:
        FrameBuffer(uint32_t length) : _length(length)
        {
            _data = new uint8_t[_length];
        }

        ~FrameBuffer()
        {
            if (_data != nullptr)
            {
                delete[] _data;
            }
        }

        inline uint8_t* data() { return _data; }
        inline uint32_t length() const { return _length; }

        virtual void set_data(void* data, uint32_t size) = 0;
        virtual int width() const = 0;
        virtual int height() const = 0;

    protected:
        uint8_t* _data = nullptr;
        uint32_t _length;
    };

    class FullFrameBuffer : public FrameBuffer
    {
    public:
        FrameBuffer(int width, int height) : FrameBuffer(width*height) {}
        void set_data(void* data, uint32_t size, int width) override
        {
            size_t inc = 0;

            size_t end = min(_length*2, size);

            for (size_t k = 0; k < end; k += 2, ++inc)
            {
                _data[inc] = data[k];
            }
        }

        inline int width() const override { return _width; }
        inline int height() const override { return _height; }

    private:
        int _width;
        int _height;
    };

    class CroppedFrameBuffer : public FrameBuffer
    {
    public:
        CroppedFrameBuffer(CropWindow* win) : _win(win), FrameBuffer(win->length()) {}

        void set_data(void* data, size_t size, int width)
        {
            int inc = 0;
            int w2 = width * 2;

            for (int k = _win.row; k < _win.height; ++k, ++inc)
            {
                for (int j = (_win.col * 2); j < (_win.width*2); j+=2)
                {
                    int idx = (k * w2 + j);
                    if (idx < size)
                    {
                        _data[inc] = data[idx];
                    }
                }
            }
        }

        inline int width() const { return _win->width; }
        inline int height() const { return _win->height; }

    private:
        CropWindow* _win;
    };
}
