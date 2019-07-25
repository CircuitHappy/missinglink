#include <iostream>
#include <cstdlib>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <rtmidi/RtMidi.h>

namespace MissingLink {

class MidiIn {

  public:

    MidiIn();
    virtual ~MidiIn();

    void CheckPorts();

    std::function<void(double)> onNewTempo;
    std::function<void()> onStartTransport;

  protected:

    typedef std::chrono::time_point<std::chrono::steady_clock> timestamp;

    std::vector<unsigned char> m_message;
    unsigned int m_numPorts;

    std::atomic<bool> m_block_midi;

    std::vector<std::shared_ptr<RtMidiIn>> m_ports; //repository for all the known hardware ports, port 0 is internal software port

    void clockInPerQn();
    void clockInPerTick();
    void startTransport();
    void continueTransport();
    void stopTransport();
    static void messageCallback( double deltatime, std::vector< unsigned char > *message, void *userData );
    unsigned int CountPorts();
    void init_ports();
    void close_ports();

};

}//namespaces
