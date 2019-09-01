template <class T>
class Packet
{
public:
    Packet(T data) : _data(data) {}
    Packet() = default;
    T data() { return _data; }
protected:
    T _data;
};

template <class PacketType>
class Sink
{
public:
    virtual bool open_stream() = 0;
    virtual bool close_stream() = 0;
    virtual void process(PacketType& sig) = 0;
};

template <class PacketType>
class Source
{
public:
    virtual bool open_stream() = 0;
    virtual bool start_stream() = 0;
    virtual bool stop_stream() = 0;
    virtual bool close_stream() = 0;

    void register_sink(Sink<SingalType>* sink) { _sink = sink; }
protected:
    Sink<PacketType>* _sink;
};


class FileSink : public Sink
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
