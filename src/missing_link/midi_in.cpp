// midiin.cpp
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <string>
#include "missing_link/midi_in.hpp"

using namespace std::chrono;
using namespace MissingLink;

MidiIn::MidiIn()
  : m_numPorts(0)
  , m_block_midi(true)
  , m_ports()
{
  init_ports();
}

MidiIn::~MidiIn() {
  m_block_midi = true;
  close_ports();
}

void MidiIn::messageCallback( double deltatime, std::vector< unsigned char > *message, void *userData ) {
  //delta time seems to be 0.0 for start/stop/clock messages
  //unsigned int nBytes = message->size();
  MidiIn *midiIn = static_cast<MidiIn *>(userData);
  switch ((int)message->at(0)) {
    case 248:
      std::cout << "clock tick delta: " << deltatime << std::endl;
      midiIn->clockInPerTick();
      break;
    case 250:
      midiIn->startTransport();
      break;
    case 251:
      midiIn->continueTransport();
      break;
    case 252:
      midiIn->stopTransport();
      break;
    default:
      break;
  }
}

void MidiIn::clockInPerQn() {
  //receive MIDI Clock Here
  const auto now = steady_clock::now();
  static int clockCount = 0;
  static auto startClockTime = now;

  clockCount += 1;

  // if (intervalSinceLast >= milliseconds(5000)) {
  //   clockCount = 1;
  // }

  if (clockCount == 1) {
    startClockTime = now;
  }

  if (clockCount == 24) {
    const auto totalInterval = now - startClockTime;
    const double newTempo = (int)((seconds(60)/totalInterval) + 0.5);

    if (onNewTempo) {
      onNewTempo(newTempo);
    }
    clockCount = 0;
  }
}

void MidiIn::clockInPerTick() {
  //receive MIDI Clock Here
  const auto now = steady_clock::now();
  static int clockCount = 0;
  static auto lastClockTime = now;
  static double lastTempo = 0.0;
  double avgTempo;
  static auto prevClockInterval = now - lastClockTime;
  const auto clockInterval = now - lastClockTime;

  clockCount += 1;

  if (clockCount == 1) {
    lastClockTime = now;
    return;
  }

  if (clockCount == 2) {
    prevClockInterval = clockInterval;
  }

  const auto qn = 24 * clockInterval;
  const double newTempo = (int)((seconds(60)/qn) + 0.5);
  lastClockTime = now;
  //update tempo at second clock tick and then every quarternote afterward
  if (clockCount == 2) {
    if (onNewTempo) {
      onNewTempo(newTempo);
    }
  }
  if ((clockCount % 24) == 0) {
    avgTempo = (int)(((newTempo + lastTempo) * 0.5) + 0.5);
    if (onNewTempo) {
      onNewTempo(avgTempo);
    }
    lastTempo = avgTempo;
  }
}

void MidiIn::startTransport() {
  //react to starttransport message
  //std::cout << "MIDI In: start transport" << std::endl;
  if (onNewTempo) {
    onStartTransport();
  }
}

void MidiIn::continueTransport() {
  //react to continueTransport message
  std::cout << "MIDI In: continue transport" << std::endl;
}

void MidiIn::stopTransport() {
  //react to StopTransport message
  std::cout << "MIDI In: stop transport" << std::endl;
}

void MidiIn::CheckPorts() {
  unsigned int nPorts = CountPorts();
  if (nPorts != m_numPorts) {
    if (nPorts < m_numPorts){
      std::cout << "MIDI In: Lost MIDI interface." << std::endl;
    } else {
      //number of ports is greater than previously known
      std::cout << "MIDI In: New interface detected." << std::endl;
    }
    init_ports();
    m_numPorts = nPorts;
  }
}

unsigned int MidiIn::CountPorts() {
  unsigned int count;
  auto midi = std::shared_ptr<RtMidiIn>(new RtMidiIn());
  count = midi->getPortCount();
  midi.reset();
  return count;
}

void MidiIn::init_ports() {
  m_block_midi = true;
  close_ports();
  // Add all available ports, excluding port 0 (internal software port)
  unsigned int nPorts = CountPorts();
  if (nPorts != 1) { std::cout << "MIDI In: Found " << nPorts << " MIDI In port(s)" << std::endl; }
  m_numPorts = nPorts;
  for (unsigned int i = 2; i < nPorts; i++) {
    auto port = std::shared_ptr<RtMidiIn>(new RtMidiIn());
    if (port->getPortName(i).find("RtMidi") == std::string::npos) {
      m_ports.push_back(port);
      try {
        std::cout << "MIDI In: Trying to open port " << i << ", " << port->getPortName(i) << std::endl;
        port->openPort(i);
        port->setCallback(&messageCallback,(void *)this);
        port->ignoreTypes( false, false, false ); // Don't ignore sysex, timing, or active sensing messages.
        std::cout << "MIDI In: Port " << i << " Ready" << std::endl;
      } catch (RtMidiError &error) {
        std::cout << "Error adding port." << std::endl;
        error.printMessage();
      }
    }
  }
  m_block_midi = false;
  if (nPorts <= 1) {
    //If there's only 1 port available, that's a software port, not hardware
    std::cout << "MIDI In: No External ports available!" << std::endl;
  }
}

void MidiIn::close_ports() {
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
