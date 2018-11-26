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
    // Open Port 1 which should be the USB MIDI adapter.
    try {
      m_pMidiOut->openPort( 1 );
      m_foundMidiPort = true;
    } catch (RtMidiError &error) {
      error.printMessage();
      m_foundMidiPort = false;
    }
  }

}

void MidiOut::close() {
  //close open MIDI port
}
