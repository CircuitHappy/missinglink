// midiout.cpp
#include <iostream>
#include <cstdlib>
#include "missing_link/midi_out.hpp"

using namespace MissingLink;

MidiOut::MidiOut()
  : m_pMidiOut(std::unique_ptr<RtMidiOut>(new RtMidiOut()))
  , m_foundMidiPort(false)
{
  open();
}

MidiOut::~MidiOut() {
  close();
}

void MidiOut::ClockOut() {
  //output clock messages
  m_message.clear();
  m_message.push_back( 0xF8 );
  m_pMidiOut->sendMessage( &m_message );
}
void MidiOut::StartTransport() {
  //output Start Transport messages
  m_message.clear();
  m_message.push_back( 0xFA );
  m_pMidiOut->sendMessage( &m_message );
}
void MidiOut::StopTransport() {
  //output Stop Transport messages
  m_message.clear();
  m_message.push_back( 0xFC );
  m_pMidiOut->sendMessage( &m_message );
}
void MidiOut::AllNotesOff() {
  //output All Notes Off messages
}

void MidiOut::open() {
  // Check available ports.
  unsigned int nPorts = m_pMidiOut->getPortCount();
  std::cout << "Found " << nPorts << " MIDI port(s)\n";
  if ( nPorts == 0 ) {
    std::cout << "No MIDI ports available!\n";
    m_foundMidiPort = false;
  } else {
    // Open first available port.
    m_pMidiOut->openPort( 1 );
    m_foundMidiPort = true;
  }

}

void MidiOut::close() {
  //close open MIDI port
}

// int main()
// {
//   RtMidiOut *midiout = new RtMidiOut();
//   std::vector<unsigned char> message;
//   // Check available ports.
//   unsigned int nPorts = midiout->getPortCount();
//   if ( nPorts == 0 ) {
//     std::cout << "No ports available!\n";
//     goto cleanup;
//   }
//   // Open first available port.
//   midiout->openPort( 0 );
//   // Send out a series of MIDI messages.
//   // Program change: 192, 5
//   message.push_back( 192 );
//   message.push_back( 5 );
//   midiout->sendMessage( &message );
//   // Control Change: 176, 7, 100 (volume)
//   message[0] = 176;
//   message[1] = 7;
//   message.push_back( 100 );
//   midiout->sendMessage( &message );
//   // Note On: 144, 64, 90
//   message[0] = 144;
//   message[1] = 64;
//   message[2] = 90;
//   midiout->sendMessage( &message );
//   SLEEP( 500 ); // Platform-dependent ... see example in tests directory.
//   // Note Off: 128, 64, 40
//   message[0] = 128;
//   message[1] = 64;
//   message[2] = 40;
//   midiout->sendMessage( &message );
//   // Clean up
//  cleanup:
//   delete midiout;
//   return 0;
// }
