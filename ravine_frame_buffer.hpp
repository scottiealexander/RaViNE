#ifndef RAVINE_BUFFER_HPP_
#define RAVINE_BUFFER_HPP_

#define min(x, y) ((x) < (y) ? (x) : (y))

namespace ravine
{
    /* ====================================================================== */
    struct CropWindow
    {
        int col;
        int row;
        int width;
        int height;
        inline uint32_t length() const
        {
            return ((uint32_t)width) * ((uint32_t)height);
        }
    }
    /* ====================================================================== */
    class FrameBuffer
    {
    public:
        FrameBuffer(uint32_t length);
        ~FrameBuffer();

        inline uint8_t* data() { return _data; }
        inline uint32_t length() const { return _length; }

        virtual void set_data(void* data, uint32_t size) {};
        virtual int width() const { return 0; };
        virtual int height() const { return 0; };

    protected:
        uint8_t* _data = nullptr;
        uint32_t _length;
    };
    /* ====================================================================== */
    class FullFrameBuffer : public FrameBuffer
    {
    public:
        FullFrameBuffer(int width, int height) :
            FrameBuffer(width*height), _width(width), _height(height) {}

        void set_data(void* data, uint32_t size, int width) override;

        inline int width() const override { return _width; }
        inline int height() const override { return _height; }

    private:
        int _width;
        int _height;
    };
    /* ====================================================================== */
    class CroppedFrameBuffer : public FrameBuffer
    {
    public:
        CroppedFrameBuffer(CropWindow* win) : _win(win), FrameBuffer(win->length()) {}

        void set_data(void* data, uint32_t size, int width) override;

        inline int width() const override { return _win->width; }
        inline int height() const override { return _win->height; }

    private:
        CropWindow* _win;
    };
    /* ====================================================================== */
}
#endif
