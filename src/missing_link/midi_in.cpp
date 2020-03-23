// midiin.cpp
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <ratio>
#include <string>
#include "missing_link/engine.hpp"
#include "missing_link/midi_in.hpp"

#include <typeinfo>

using namespace std::chrono;
using namespace MissingLink;

MidiInProcess::MidiInProcess(Engine &engine)
  : Engine::Process(engine, std::chrono::microseconds(100))
  , m_numPorts(0)
  , m_block_midi(true)
  , m_clockCount(0)
  , m_ports()
{
  init_ports();
}

// MidiIn::~MidiIn() {
//   m_block_midi = true;
//   close_ports();
// }

void MidiInProcess::Run() {
  Process::Run();
  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(m_pThread->native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }
}

void MidiInProcess::process() {
  int nBytes, portCt;
  std::vector<unsigned char> message;
  high_resolution_clock::time_point now;
  duration<double> time_span;
  //std::cout << "Port count: " << m_ports.size() << std::endl;
  portCt = 0;
  for(auto & port : m_ports) {
    //std::cout << "port (" << portCt << ") " << typeid(port).name() << std::endl;
    try {
      port->getMessage(&message);
      nBytes = message.size();
      if (nBytes > 1) {
        std::cout << "nBytes: " << nBytes << std::endl;
      }
      for ( int i=0; i<nBytes; i++ ){
        switch ((int)message[i]) {
          // case: clock pulse
          case 248:
            now = high_resolution_clock::now();
            time_span = duration_cast<duration<double>>(now - m_prevClockTime);
            m_prevClockTime = now;
            clockInAvgCount(time_span.count());
            //clockInPerQn(time_span.count());
            break;
          case 250:
            startTransport();
            break;
          case 251:
            continueTransport();
            break;
          case 252:
            stopTransport();
            break;
          default:
            break;
        }
      }
    } catch (RtMidiError &error) {
      error.printMessage();
    }
    portCt ++;
  }
}

void MidiInProcess::clockInPerQn(double deltatime) {
  //based on rtmidi tests/midiclock.cpp
  static int clockCount = 0;
  clockCount += 1;

  if (clockCount == 24) {
    double bpm = 60.0 / 24.0 / deltatime;

    if (onNewTempo) {
      onNewTempo(bpm);
    }
    clockCount = 0;
  }
}

void MidiInProcess::clockInAvgCount(double deltatime) {
  //based on rtmidi tests/midiclock.cpp
  //but average each deltatime to smooth it out
  static double deltaAvg = 0;
  static double prevTempo = 0;
  static double avgTempo = 0;
  static int tempoStableCount = 0;
  m_clockCount += 1;
  if (m_clockCount == 1) {
    deltaAvg = deltatime;
    //perhaps re-align the Link grid here
  }

  deltaAvg = (deltaAvg + deltatime) * 0.5;

  if (m_clockCount == 24) {
    double bpm = (int)(60.0 / 24.0 / deltaAvg);

    if (abs(avgTempo - bpm) < 5) {
      avgTempo = (int)(((avgTempo + bpm) * 0.5) + 0.5);
    } else {
      avgTempo = bpm;
    }

    if (prevTempo == avgTempo) {
      tempoStableCount ++;
    } else {
      tempoStableCount = 0;
    }

    if (tempoStableCount == 4) {
      if (onNewTempo) {
        onNewTempo(avgTempo);
        tempoStableCount = 0;
      }
    }
    prevTempo = avgTempo;
    m_clockCount = 0;
  }
}

void MidiInProcess::clockInAvgDelta(double deltatime) {
  //based on rtmidi tests/midiclock.cpp
  //but average each deltatime to smooth it out
  static double deltaAvg = 0;
  static double avgTempo = 0;
  m_clockCount += 1;
  if (m_clockCount == 1) {
    deltaAvg = deltatime;
    //perhaps re-align the Link grid here
  }

  deltaAvg = (deltaAvg + deltatime) * 0.5;

  if (m_clockCount == 24) {
    double bpm = (int)(60.0 / 24.0 / deltaAvg);

    if (abs(avgTempo - bpm) < 5) {
      avgTempo = (int)(((avgTempo + bpm) * 0.5) + 0.5);
    } else {
      avgTempo = bpm;
    }

    if (onNewTempo) {
      onNewTempo(avgTempo);
    }
    m_clockCount = 0;
  }
}

void MidiInProcess::clockInPerTick() {
  //adapted from tap_tempo.cpp
  const auto now = steady_clock::now();
  static int clockCount = 0;
  static auto lastClockTime = now;
  static double lastTempo = 0.0;
  double avgTempo;
  static auto prevClockInterval = now - lastClockTime;
  const auto clockInterval = now - lastClockTime;

  clockCount += 1;

  if (clockCount == 1) {
    //we only have one clock pulse. Not enough to measure tempo.
    lastClockTime = now;
    return;
  }

  if (clockCount == 2) {
    prevClockInterval = clockInterval;
  }

  const auto qn = 24 * clockInterval;
  const double newTempo = (int)((seconds(60)/qn) + 0.5);
  lastClockTime = now;
  // update tempo at second clock tick, to get some tempo going quickly
  if (clockCount == 2) {
    if (onNewTempo) {
      onNewTempo(newTempo);
    }
  }
  // and then update tempo every quarternote afterward
  if ((clockCount % 24) == 0) {
    //average with previous tempo in hopes of smoothing things out
    avgTempo = (int)(((newTempo + lastTempo) * 0.5) + 0.5);
    if (onNewTempo) {
      onNewTempo(avgTempo);
    }
    lastTempo = avgTempo;
  }
}

void MidiInProcess::startTransport() {
  // react to starttransport message
  std::cout << "MIDI In: start transport" << std::endl;
  if (onNewTempo) {
    onStartTransport();
  }
}

void MidiInProcess::continueTransport() {
  //react to continueTransport message
  std::cout << "MIDI In: continue transport" << std::endl;
}

void MidiInProcess::stopTransport() {
  //react to StopTransport message
  std::cout << "MIDI In: stop transport" << std::endl;
}

void MidiInProcess::CheckPorts() {
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

unsigned int MidiInProcess::CountPorts() {
  unsigned int count;
  auto midi = std::shared_ptr<RtMidiIn>(new RtMidiIn());
  count = midi->getPortCount();
  midi.reset();
  return count;
}

void MidiInProcess::init_ports() {
  m_block_midi = true;
  close_ports();
  // Add all available ports, excluding port 0 (internal software port)
  unsigned int nPorts = CountPorts();
  if (nPorts != 1) { std::cout << "MIDI In: Found " << nPorts << " MIDI In port(s)" << std::endl; }
  m_numPorts = nPorts;
  for (unsigned int i = 1; i < nPorts; i++) {
    auto port = std::shared_ptr<RtMidiIn>(new RtMidiIn());
    if (port->getPortName(i).find("RtMidi") == std::string::npos) {
      m_ports.push_back(port);
      try {
        std::cout << "MIDI In: Trying to open port " << i << ", " << port->getPortName(i) << std::endl;
        port->openPort(i);
        //port->setCallback(&messageCallback,(void *)this);
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

void MidiInProcess::close_ports() {
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
