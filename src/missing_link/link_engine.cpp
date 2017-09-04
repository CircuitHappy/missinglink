#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "missing_link/pin_defs.hpp"
#include "missing_link/link_engine.hpp"

#define SOCK_PATH "/tmp/ml-display-bus"

using namespace std;
using namespace MissingLink;

LinkEngine::Pins::Pins()
  : clockOut(GPIO::SysfsPin(GPIO::CHIP_D0, GPIO::Pin::OUT))
  , resetOut(GPIO::SysfsPin(GPIO::CHIP_D1, GPIO::Pin::OUT))
  , playingOut(GPIO::SysfsPin(GPIO::CHIP_D6, GPIO::Pin::OUT))
  , playStopIn(GPIO::SysfsPin(GPIO::CHIP_D7, GPIO::Pin::IN))
{}

void LinkEngine::Pins::ExportAll() {
  clockOut.Export();
  resetOut.Export();
  playingOut.Export();
  playStopIn.Export();
}

void LinkEngine::Pins::UnexportAll() {
  clockOut.Unexport();
  resetOut.Unexport();
  playingOut.Unexport();
  playStopIn.Unexport();
}

LinkEngine::State::State(LinkEngine::Pins &pins)
  : link(120.0)
  , running(true)
  , playState(Stopped)
  , playStop(GPIO::Button(pins.playStopIn))
  , pins(pins)
{
  link.enable(true);
}

void LinkEngine::Run() {
  Pins pins;
  State state(pins);

  pins.UnexportAll();
  pins.ExportAll();

  std::thread inputThread(&LinkEngine::runInput, this, std::ref(state));
  std::thread outputThread(&LinkEngine::runOutput, this, std::ref(state));

  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(outputThread.native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }

  runDisplaySocket(state);

  inputThread.join();
  outputThread.join();

  pins.UnexportAll();
}

void LinkEngine::runInput(LinkEngine::State &state) {

  while (state.running) {

    state.playStop.Process();

    if (state.playStop.GetCurrentEvent() == GPIO::Button::TOUCH_DOWN) {
      switch ((PlayState)state.playState) {
        case Stopped:
          state.playState.store(Cued);
          break;
        case Cued:
        case Playing:
          state.playState.store(Stopped);
          break;
      }
    }

    this_thread::sleep_for(chrono::milliseconds(10));
  }

}

void LinkEngine::runOutput(LinkEngine::State &state) {
  while (state.running) {
    const auto time = state.link.clock().micros();
    auto timeline = state.link.captureAudioTimeline();

    const double beats = timeline.beatAtTime(time, QUANTUM);
    const double phase = timeline.phaseAtTime(time, QUANTUM);
    const double tempo = timeline.tempo();

    switch ((PlayState)state.playState) {
      case Cued: {
        const bool playHigh = (long)(beats * 2) % 2 == 0;
        state.pins.playingOut.Write(playHigh ? HIGH : LOW);
        if (phase <= 0.01) {
          state.playState.store(Playing);
        }
        break;
      }
      case Playing:
        state.pins.playingOut.Write(HIGH);
        outputClock(state.pins, beats, phase, tempo);
        break;
      default:
        state.pins.playingOut.Write(LOW);
        state.pins.clockOut.Write(LOW);
        state.pins.resetOut.Write(LOW);
        break;
    }

    this_thread::sleep_for(chrono::microseconds(250));
  }
}

void LinkEngine::runDisplaySocket(LinkEngine::State &state) {
  // Proof of concept display code
  int sd;
  struct sockaddr_un remote;

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  int sd_len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  while (state.running) {

    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      cerr << "failed to create display socket\n";
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    if (connect(sd, (struct sockaddr *)&remote, sd_len) == -1) {
      cerr << "failed to connect to display socket\n";
      close(sd);
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    auto timeline = state.link.captureAppTimeline();
    const double tempo = timeline.tempo();

    char display_buf[8];
    sprintf(display_buf, "%.1f", tempo);

    if (send(sd, display_buf, 8, 0) < 0) {
      cerr << "failed to send to display socket\n";
    }

    close(sd);

    this_thread::sleep_for(chrono::milliseconds(50));
  }
}

void LinkEngine::outputClock(Pins &pins, double beats, double phase, double tempo) {
  // Fractional portion of current beat value
  double intgarbage;
  const double beatFraction = std::modf(beats * PULSES_PER_BEAT, &intgarbage);

  const double secondsPerPhrase = 60.0 / (tempo / QUANTUM);
  const double resetHighFraction = PULSE_LENGTH / secondsPerPhrase;
  const bool resetHigh = (phase <= resetHighFraction);
  pins.resetOut.Write(resetHigh ? HIGH : LOW);

  // Fractional beat value for which clock should be high
  const double clockHighFraction = 0.5;
  const bool clockHigh = (beatFraction <= clockHighFraction);
  pins.clockOut.Write(clockHigh ? HIGH : LOW);
}
