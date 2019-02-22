// midiout.cpp
#include <iostream>
#include <cstdlib>
#include "missing_link/midi_out.hpp"

using namespace MissingLink;

MidiOut::MidiOut()
  : m_pMidiOut(std::unique_ptr<RtMidiOut>(new RtMidiOut()))
  , m_foundMidiPort(false)
  , m_numPorts(1)
{
  open();
}

MidiOut::~MidiOut() {
  close();
}

void MidiOut::ClockOut() {
  if (m_foundMidiPort) {
    //output clock messages
    m_message.clear();
    m_message.push_back( 0xF8 );
    try {
      m_pMidiOut->sendMessage( &m_message );
    } catch (RtMidiError &error) {
      error.printMessage();
      close();
    }
  }
}
void MidiOut::StartTransport() {
  if (m_foundMidiPort) {
    //output Start Transport messages
    m_message.clear();
    m_message.push_back( 0xFA );
    try {
      m_pMidiOut->sendMessage( &m_message );
    } catch (RtMidiError &error) {
      error.printMessage();
      close();
    }
  }
}
void MidiOut::StopTransport() {
  if (m_foundMidiPort) {
    //output Stop Transport messages
    m_message.clear();
    m_message.push_back( 0xFC );
    try {
      m_pMidiOut->sendMessage( &m_message );
    } catch (RtMidiError &error) {
      error.printMessage();
      close();
    }
  }
}
void MidiOut::AllNotesOff() {
  if (m_foundMidiPort) {
  //output All Notes Off messages
  }
}

void MidiOut::CheckPorts() {
  unsigned int nPorts = m_pMidiOut->getPortCount();
  if (nPorts != m_numPorts) {
    if (nPorts < m_numPorts){
      std::cout << "Lost MIDI interface." << std::endl;
      close();
    } else {
      //number of ports is greater than previously known
      std::cout << "New MIDI interface detected." << std::endl;
      open();
    }
    m_numPorts = nPorts;
  }
}

void MidiOut::open() {
  // Check available ports.
  unsigned int nPorts = m_pMidiOut->getPortCount();
  m_foundMidiPort = false;
  std::cout << "Found " << nPorts << " MIDI port(s)" << std::endl;
  if ( nPorts > 1 ) {
    m_numPorts = nPorts;
    // Open Port 1 which should be the USB MIDI adapter.
    try {
      std::cout << "Trying to open port 1" << std::endl;
      m_pMidiOut->openPort( 1 );
      m_foundMidiPort = true;
      std::cout << "Port 1 Ready" << std::endl;
    } catch (RtMidiError &error) {
      error.printMessage();
    }
  } else {
    //If there's only 1 port available, that's a software port, not hardware
    std::cout << "No External MIDI ports available!" << std::endl;
    m_foundMidiPort = false;
  }
}

void MidiOut::close() {
  m_foundMidiPort = false;
  //close open MIDI port
  std::cout << "Closing open MIDI port." << std::endl;
  try {
    m_pMidiOut->closePort();
  } catch (RtMidiError &error) {
    error.printMessage();
  }
}
