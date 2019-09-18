# RaViNE
**RA**spberry pi
**VI**sual
**N**europhysiology
**E**mulator

## Requirements
* Linux OS (for v4l2)
* GCC / G++ that support C++11 (tested on 7.4.0)
* [Portaudio v19](http://www.portaudio.com/download.html)
    * tested using the v19 daily snapshot, though the stable release should also work (I think...)
    * pa_ringbuffer (see **Building** below)
* [ASIO](https://think-async.com/Asio/)
    * Tested with [v1.12.2](https://sourceforge.net/projects/asio/files/asio/1.12.2%20%28Stable%29/)

## Building
1. Build portaudio: [instructions](http://portaudio.com/docs/v19-doxydocs/compile_linux.html)

2. Assuming `portaudio` and `asio` are installed in `~/libraries` and that step 1 was completed, compile `pa_ringbuffer`:
```bash
# build pa_ringbuffer, move lib to portaudio libs dir
cd "~/libraries/portaudio/src/common"
gcc -O2 -I./ -c -o pa_ringbuffer.o pa_ringbuffer.c
# not necessary but sensible
ar rcs ../../lib/.libs/libparingbuffer.a ./pa_ringbuffer.o
```

3. Compile `ravine`:
```bash
# cd to wherever you installed ravine
# set the location of portaudio and asio
export PORTAUDIO_PATH=~/libraries/portaudio
export ASIO_PATH=~/libraries/asio-1.12.2
# build ravine
make release
```

The program `ravine` can be found in `build/app`.
