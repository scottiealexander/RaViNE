#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

#include <csignal>

#include "ravine_utils.hpp"
#include "ravine_packets.hpp"
#include "ravine_video_source.hpp"
#include "ravine_event_source.hpp"
#include "ravine_audio_filter.hpp"
#include "ravine_neuron_filter.hpp"
#include "ravine_datafile_sink.hpp"

#define FRAMERATE 15

#define MICROSECONDS 1000000
#define WIDTH 320
#define HEIGHT 240
#define LEFT 96 //WIDTH - 64
#define TOP  56 //HEIGHT - 64

std::atomic_flag GLOBAL_WAIT = ATOMIC_FLAG_INIT;

void handle_signal(int /* signal */)
{
    GLOBAL_WAIT.clear();
}

inline bool keep_waiting()
{
    return GLOBAL_WAIT.test_and_set(std::memory_order_acquire);
}

/* ========================================================================= */
int main(int narg, const char** args)
{
    int EXIT_CODE = 0;

    signal(SIGINT, handle_signal);
    (void)keep_waiting();

    const char* dev = "/dev/video0";

    int port = -1;
    bool save = false;
    bool listen = false;
    std::string ofile;
    std::string rffile("./rf/rf-05.pgm");

    int k = 1;
    while (k < narg)
    {
        std::string tmp(args[k]);

        if (tmp == "-p")
        {
            if (narg > (k + 1))
            {
                port = std::atoi(args[k+1]);
                listen = true;
                k += 2;
            }
        }
        else if (tmp == "-f")
        {
            if (narg > (k + 1))
            {
                ofile.assign(args[k+1]);
                save = true;
                k += 2;
            }
            else { printf("WTF\n"); }
        }
        else if (tmp == "-r")
        {
            if (narg > (k + 1))
            {
                rffile.assign(args[k+1]);
                k += 2;
            }
        }
        else
        {
            printf("[ERROR]: invalid input \"%s\"\n", tmp.c_str());
            return -1;
        }
    }

    printf("Port: %d | save: %d | listen: %d | ofile: %s | rffile: %s\n",
        port, save, listen, ofile.c_str(), rffile.c_str());

    if (listen && (port < 1 || port > 65535))
    {
        printf("[ERROR]: invalid port %d\n", port);
        return -1;
    }

    if (save && ofile.empty())
    {
        printf("[ERROR]: cannot set save to true with out valid output file (-f)\n");
        return -1;
    }

    RVN::AudioFilter audio;

    RVN::V4L2 video(dev, WIDTH, HEIGHT, FRAMERATE);

    if (!video.open_stream())
    {
        printf("[ERROR]: failed to open stream\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        return -1;
    }

    if (!video.initialize_buffers(16))
    {
        printf("[ERROR]: failed to init buffers\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        return -1;
    }

    RVN::NeuronFilter neuron(rffile.c_str(), LEFT, TOP, 8);

    if (!neuron.isvalid())
    {
        printf("[ERROR]: failed to initialize neuron filter\n");
        printf("    [MSG]: %s\n", neuron.get_error_msg().c_str());
        return -1;
    }

    RVN::DataFileSink* datafile = nullptr;
    if (save)
    {
        datafile = new RVN::DataFileSink(ofile.c_str(), audio.frames_per_buffer);

        if (!datafile->isvalid())
        {
            printf("[ERROR]: failed to init sink\n");
            printf("[MSG]: %s\n", datafile->get_error_msg().c_str());
            delete datafile;
            return -1;
        }
    }

    RVN::EventSource* events = nullptr;
    if (listen)
    {
        events = new RVN::EventSource(port);

        if (save)
        {
            // prep the event source -> datafile sink stream
            events->register_sink(datafile);

            if (!events->has_valid_sink())
            {
                printf("[ERROR]: failed to register sink for event source\n");
                delete events;
                return -1;
            }
        }

        // start accepting connections asap!
        if (!events->open_stream())
        {
            printf("[ERROR]: Failed to open stream\n");
            EXIT_CODE = -1;
            goto error;
        }
    }

    if (save)
    {
        // while we wait for event_source connection, set up audio system:
        // register the file sink with the audio filter / source
        audio.register_sink(datafile);

        if (!audio.has_valid_sink())
        {
            printf("[ERROR]: failed to register sink for audio filter\n");
            EXIT_CODE = -1;
            goto error;
        }
    }

    neuron.register_sink(&audio);

    if (!neuron.has_valid_sink())
    {
        printf("[ERROR]: failed to register sink with source\n");
        EXIT_CODE = -1;
        goto error;
    }

    video.register_sink(&neuron);

    if (!video.has_valid_sink())
    {
        printf("[ERROR]: failed to register sink with source\n");
        EXIT_CODE = -1;
        goto error;
    }

    printf("[MAIN]: everything seems to be working...\n");

    if (listen)
    {
        if (!events->start_stream())
        {
            printf("[ERROR]: failed to start event stream\n");
            EXIT_CODE = -1;
            goto error;
        }

        //all ready (hopefully..?) wait for connection from our external event
        //generator
        while (!events->isready())
        {
            RVN::sleep_ms(10);
        }
    }

    if (!video.start_stream())
    {
        printf("[ERROR]: failed to start stream\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        EXIT_CODE = -1;
        goto error;
    }

    printf("[MAIN]: entering main loop!\n");
    if (listen)
    {
        while (events->still_running())
        {
            RVN::sleep_ms(100);
        }
    }
    else
    {
        while (keep_waiting())
        {
            RVN::sleep_ms(500);
        }
    }

    printf("[MAIN]: exiting main loop!\n");

    printf("[MAIN]: stopping video stream!\n");
    if (!video.stop_stream())
    {
        printf("[ERROR]: error during streaming\n");
        printf("[MSG]: %s\n", video.get_error_msg().c_str());
        video.close_stream();
        EXIT_CODE = -1;
        goto error;
    }

    printf("[MAIN]: closing video stream!\n");
    if (!video.close_stream())
    {
        printf("[ERROR]: failed to close stream?\n");
        EXIT_CODE = -1;
        goto error;
    }

    if (listen)
    {
        printf("[MAIN]: closing event stream!\n");
        if (!events->close_stream())
        {
            printf("[ERROR]: Failed to close event stream\n");
            EXIT_CODE = -1;
        }
    }

error:

    if (datafile != nullptr) { delete datafile; }
    if (events != nullptr) { delete events; }

    return EXIT_CODE;
}
