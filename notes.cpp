//

// class Device
// {
//   Device() {}
//   ~Device() {}
//   void open() {}
//   void close() {}
// };

side_length = 4
nrow = 8

idx = ((nrow / side_length)*floor.(Int, x / side_length)) .+ floor.(Int, y / side_length)

class BaseSink
{
    BaseSink() {}
    ~BaseSink() {}
    void open_stream() {}
    void close_stream() {}
    void send() {}
};

class FileSink : public BaseSink
{
    
};

class BaseSource
{
    BaseSource() {}
    ~BaseSource() {}
    void open_stream() {}
    void start_stream() {}
    void stop_stream() {}
    void close_stream() {}
    void register_sink() {}
    
    // calls register_sink()
    void operator>>(Sink&) {}
};

class VideoSource : public BaseSource
{
    
};

class EventSource : public BaseSource
{
    
};

class BaseFilter : public BaseSource, public BaseSink
{
    BaseFilter() {}
    ~BaseFilter() {}
    void register_source()
};

class AudioFilter : public BaseFilter
{
    
};

class NeuronFilter : public BaseFilter
{
    
};

// source >> filter >> sink
// video >> neuron >> audio >> file;
// event >> file;
