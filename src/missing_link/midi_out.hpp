#include <iostream>
#include <cstdlib>
#include <memory>
#include <chrono>
#include <atomic>
#include <rtmidi/RtMidi.h>

namespace MissingLink {

class MidiOut {

  public:

    MidiOut();
    virtual ~MidiOut();

    void ClockOut();
    void StartTransport();
    void StopTransport();
    void AllNotesOff();
    void CheckPorts();

  protected:

    std::vector<unsigned char> m_message;
    unsigned int m_numPorts;

    std::atomic<bool> m_block_midi;

    std::vector<std::shared_ptr<RtMidiOut>> m_ports; //repository for all the known hardware ports, port 0 is internal software port

    unsigned int CountPorts();
    void init_ports();
    void close_ports();

};

}//namespaces
