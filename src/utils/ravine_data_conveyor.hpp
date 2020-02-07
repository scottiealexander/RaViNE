#ifndef RAVINE_DATA_CONVEYOR_HPP_
#define RAVINE_DATA_CONVEYOR_HPP_

#include <cstdlib>
#include <cstdio>

extern "C"
{
#include "pa_ringbuffer.h"
}

namespace RVN
{
    /* ====================================================================== */
    template <class T>
    class RingBuffer
    {
    public:
        /* ------------------------------------------------------------------ */
        RingBuffer(int length) : _length(length)
        {
            _data = malloc(sizeof(T*) * _length);
            if (_data != nullptr)
            {
                if (PaUtil_InitializeRingBuffer(&_buffer, sizeof(T*), _length, _data) < 0)
                {
                    free(_data);
                    _data = nullptr;
                }
            }
        }
        /* ------------------------------------------------------------------ */
        ~RingBuffer()
        {
            if (_data != nullptr)
            {
                free(_data);
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
            // we pass the address of the item we want to write to the
            // ringbuffer, the item is a T* thus &data...
            return PaUtil_WriteRingBuffer(&_buffer, &data, 1);
        }
        /* ------------------------------------------------------------------ */
        inline T* read()
        {
            // as above, we pass the address of where we want the item to be
            // writen to, the item is a T*
            T* data;
            PaUtil_ReadRingBuffer(&_buffer, &data, 1);
            return data;
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
            printf("[INFO]: ~DataConveyor...\n");
            while (load_ready())
            {
                printf("    [INFO]: deleting item from _unloaded queue\n");
                T* ptr = pop_load();
                delete ptr;
            }

            while (unload_ready())
            {
                printf("    [INFO]: deleting item from _loaded queue\n");
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

            printf("[INFO]: _unloaded queue holds %d items\n", _unloaded.read_available());
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
