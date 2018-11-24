#include <iostream>
#include <cstdlib>
#include <memory>
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

  protected:

    std::unique_ptr<RtMidiOut> m_pMidiOut;
    std::vector<unsigned char> m_message;
    bool m_foundMidiPort;

    void open();
    void close();

};

}//namespaces
