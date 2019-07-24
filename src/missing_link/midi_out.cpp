// midiout.cpp
#include <iostream>
#include <cstdlib>
#include <string>
#include "missing_link/midi_out.hpp"

using namespace MissingLink;

MidiOut::MidiOut()
  : m_numPorts(1)
  , m_block_midi(true)
  , m_ports()
{
  init_ports();
}

MidiOut::~MidiOut() {
  m_block_midi = true;
  close_ports();
}

void MidiOut::ClockOut() {
  //send clock to all open hardware ports (ignore port 0, so numPorts needs to be 2 or more)
  if (m_block_midi || (m_numPorts < 2)) {
    return;
  }
  m_message.clear();
  m_message.push_back( 0xF8 );
  for(auto & port : m_ports) {
    try {
      port->sendMessage( &m_message );
    } catch (RtMidiError &error) {
      error.printMessage();
      init_ports();
      break;
    }
  }
}

void MidiOut::StartTransport() {
  //send Start Transport to all open hardware ports (ignore port 0, so numPorts needs to be 2 or more)
  if (m_block_midi || (m_numPorts < 2)) {
    return;
  }
  m_message.clear();
  m_message.push_back( 0xFA );
  for(auto & port : m_ports) {
    try {
      port->sendMessage( &m_message );
    } catch (RtMidiError &error) {
      error.printMessage();
      init_ports();
      break;
    }
  }
}

void MidiOut::StopTransport() {
  //send Stop Transport to all open hardware ports (ignore port 0, so numPorts needs to be 2 or more)
  if (m_block_midi || (m_numPorts < 2)) {
    return;
  }
  //output clock messages
  m_message.clear();
  m_message.push_back( 0xFC );
  for(auto & port : m_ports) {
    try {
      port->sendMessage( &m_message );
    } catch (RtMidiError &error) {
      error.printMessage();
      init_ports();
      break;
    }
  }
}

void MidiOut::AllNotesOff() {
  if (m_block_midi || (m_numPorts < 2)) {
    return;
  }
  //output All Notes Off messages
}

void MidiOut::CheckPorts() {
  unsigned int nPorts = CountPorts();
  if (nPorts != m_numPorts) {
    if (nPorts < m_numPorts){
      std::cout << "MIDI Out: Lost interface." << std::endl;
    } else {
      //number of ports is greater than previously known
      std::cout << "MIDI Out: New interface detected." << std::endl;
    }
    init_ports();
    m_numPorts = nPorts;
  }
}

unsigned int MidiOut::CountPorts() {
  unsigned int count;
  auto midi = std::shared_ptr<RtMidiOut>(new RtMidiOut());
  count = midi->getPortCount();
  midi.reset();
  return count;
}

void MidiOut::init_ports() {
  m_block_midi = true;
  close_ports();
  // Add all available ports, excluding port 0 (internal software port)
  unsigned int nPorts = CountPorts();
  if (nPorts != 1) { std::cout << "MIDI Out: Found " << nPorts << " MIDI port(s)" << std::endl; }
  m_numPorts = nPorts;
  for (unsigned int i = 2; i < nPorts; i++) {
    auto port = std::shared_ptr<RtMidiOut>(new RtMidiOut());
    if (port->getPortName(i).find("RtMidi") == std::string::npos) {
      m_ports.push_back(port);
      try {
        std::cout << "MIDI Out: Trying to open port " << i << ", " << port->getPortName(i) << std::endl;
        port->openPort(i);
        std::cout << "MIDI Out: Port " << i << " Ready" << std::endl;
      } catch (RtMidiError &error) {
        error.printMessage();
      }
    }
  }
  m_block_midi = false;
  if (nPorts <= 1) {
    //If there's only 1 port available, that's a software port, not hardware
    std::cout << "MIDI Out: No External ports available!" << std::endl;
  }
}

void MidiOut::close_ports() {
  for(auto & port : m_ports) {
  /* std::cout << *it; ... */
    try {
      port->closePort();
    } catch (RtMidiError &error) {
      error.printMessage();
    }
  }
  m_ports.clear();
}
