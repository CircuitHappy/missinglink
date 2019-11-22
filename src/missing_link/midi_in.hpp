#pragma once

#include <iostream>
#include <cstdlib>
#include <memory>
#include <ctime>
#include <ratio>
#include <chrono>
#include <functional>
#include <atomic>
#include "missing_link/engine.hpp"
#include <rtmidi/RtMidi.h>

namespace MissingLink {

class MidiInProcess : public Engine::Process {

  public:

    MidiInProcess(Engine &engine);
    void Run() override;

    void CheckPorts();

    std::function<void(double)> onNewTempo;
    std::function<void()> onStartTransport;

  private:
    void process() override;

  protected:

    std::chrono::high_resolution_clock::time_point m_prevClockTime;

    std::vector<unsigned char> m_message;
    unsigned int m_numPorts;
    std::atomic<bool> m_block_midi;
    unsigned int m_clockCount;

    std::vector<std::shared_ptr<RtMidiIn>> m_ports; //repository for all the known hardware ports, port 0 is internal software port

    void clockInPerQn(double deltatime);
    void clockInAvgDelta(double deltatime);
    void clockInPerTick();
    void startTransport();
    void continueTransport();
    void stopTransport();
    unsigned int CountPorts();
    void init_ports();
    void close_ports();

};

}//namespaces
