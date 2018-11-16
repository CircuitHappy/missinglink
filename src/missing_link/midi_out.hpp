#include <iostream>
#include <cstdlib>

#include "../../vendor/rtmidi/RtMidi.h"

namespace MissingLink {
namespace MidiOut {

class MidiOut {

  public:

    MidiOut();
    virtual ~MidiOut();

    void ClockOut();
    void StartTransport();
    void StopTransport();
    void AllNotesOff();

  protected:

    RtMidiOut m_midiout;
    std::vector<unsigned char> m_message;
    bool m_foundMidiPort;

    void open();
    void close();

};

}}//namespaces
