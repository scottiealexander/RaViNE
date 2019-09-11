#ifndef RAVINE_DATA_CONVEYOR_HPP_
#define RAVINE_DATA_CONVEYOR_HPP_

// yep, we are including the impl. to avoid any build complications
extern "C"
{
#include "pa_ringbuffer.c"
}

namespace RVN
{
    /* ====================================================================== */
    template <class T>
    class RingBuffer
    {
    public:
        /* ------------------------------------------------------------------ */
        RingBuffer(int length) : _length(legnth)
        {
            _data = PaUtil_AllocateMemory(sizeof(T*) * _length);
            if (_data != nullptr)
            {
                if (PaUtil_InitializeRingBuffer(&_buffer, sizeof(T*), _length, _data) < 0)
                {
                    PaUtil_FreeMemory(_data);
                    _data = nullptr;
                }
            }
        }
        /* ------------------------------------------------------------------ */
        ~RingBuffer()
        {
            if (_data != nullptr)
            {
                PaUtil_FreeMemory(_data);
            }
        }
        /* ------------------------------------------------------------------ */
        inline bool isvalid() { return _data != nullptr; }
        /* ------------------------------------------------------------------ */
        inline int write_available()
        {
            return PaUtil_GetRingBufferWriteAvailable(&_buffer);
        }
        /* ------------------------------------------------------------------ */
        inline int read_available()
        {
            return PaUtil_GetRingBufferReadAvailable(&_buffer);
        }
        /* ------------------------------------------------------------------ */
        inline int write(const T* data)
        {
            return PaUtil_WriteRingBuffer(&_buffer, data, 1);
        }
        /* ------------------------------------------------------------------ */
        inline T* read()
        {
            void* data;
            PaUtil_ReadRingBuffer(&_buffer, data, 1);
            return static_cast<T*>(data);
        }
        /* ------------------------------------------------------------------ */
    private:
        bool _isvalid;
        int _length;

        PaUtilRingBuffer _buffer;
        void* _data = nullptr;
    };
    /* ====================================================================== */
    template <class T>
    class DataConveyor
    {
    public:
        /* ------------------------------------------------------------------ */
        DataConveyor(int length) : _length(length), _unloaded(length), _loaded(length) {}
        /* ------------------------------------------------------------------ */
        ~DataConveyor()
        {
            while (load_ready())
            {
                T* ptr = pop_load();
                delete ptr;
            }

            while (unload_ready())
            {
                T* ptr = unload();
                delete ptr;
            }
        }
        /* ------------------------------------------------------------------ */
        inline bool isvalid()
        {
            return _unloaded.isvalid() && _loaded.isvalid();
        }
        /* ------------------------------------------------------------------ */
        // any T with a copy constructor is valid
        bool fill(const T& cloneable)
        {
            bool success = true;
            if (isvalid())
            {
                for (int k = 0; k < _length; ++k)
                {
                    if (_unloaded.write_available() > 0)
                    {
                        _unloaded.write(new T(cloneable));
                    }
                    else
                    {
                        success = false;
                    }
                }
            }
            else
            {
                success = false;
            }
            return success;
        }
        /* ------------------------------------------------------------------ */
        // consumer interface
        inline bool unload_ready() { return _loaded.read_available() > 0; }
        inline T* unload() { return _loaded.read(); }
        inline void reload(T* item) { _unloaded.write(item); }
        /* ------------------------------------------------------------------ */
        // producer interface
        inline bool load_ready() { return _unloaded.read_available() > 0; }
        inline T* pop_load() { return _unloaded.read(); }
        inline void push_load(T* item) { _loaded.write(item); }
        /* ------------------------------------------------------------------ */
    private:
        int _length;
        RingBuffer<T> _unloaded;
        RingBuffer<T> _loaded;
    };
    /* ====================================================================== */
}
#endif
