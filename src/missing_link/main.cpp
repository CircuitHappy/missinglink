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

#include <ableton/Link.hpp>
#include "missing_link/pin_defs.hpp"
#include "missing_link/gpio.hpp"

#define SOCK_PATH "/tmp/ml-display-bus"

using namespace std;
using namespace MissingLink;

namespace {

  const double PULSES_PER_BEAT = 4.0;
  const double PULSE_LENGTH = 0.030; // seconds
  const double QUANTUM = 4;

  struct Pins {

    GPIO::SysfsPin clockOut;
    GPIO::SysfsPin resetOut;
    GPIO::SysfsPin playingOut;
    GPIO::SysfsPin playStopIn;

    Pins()
      : clockOut(GPIO::SysfsPin(GPIO::CHIP_D0, GPIO::Pin::OUT))
      , resetOut(GPIO::SysfsPin(GPIO::CHIP_D1, GPIO::Pin::OUT))
      , playingOut(GPIO::SysfsPin(GPIO::CHIP_D6, GPIO::Pin::OUT))
      , playStopIn(GPIO::SysfsPin(GPIO::CHIP_D7, GPIO::Pin::IN))
    {}

    void ExportAll() {
      clockOut.Export();
      resetOut.Export();
      playingOut.Export();
      playStopIn.Export();
    }

    void UnexportAll() {
      clockOut.Unexport();
      resetOut.Unexport();
      playingOut.Unexport();
      playStopIn.Unexport();
    }
  };

  enum PlayState {
      Stopped,
      Cued,
      Playing
  };

  struct State {
      ableton::Link link;
      std::atomic<bool> running;
      std::atomic<PlayState> playState;

      GPIO::Button playStop;

      Pins &pins;

      State(Pins &pins)
        : link(120.0)
        , running(true)
        , playState(Stopped)
        , playStop(GPIO::Button(pins.playStopIn))
        , pins(pins)
      {
        link.enable(true);
      }
  };

  void outputClock(Pins &pins, double beats, double phase, double tempo) {

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

  void input(State& state) {
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

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void output(State& state) {
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

      std::this_thread::sleep_for(std::chrono::microseconds(250));
    }
  }
}

Pins pins;
State state(pins);

int main(void) {

  state.pins.UnexportAll();
  state.pins.ExportAll();

  std::thread inputThread(input, std::ref(state));
  std::thread outputThread(output, std::ref(state));

  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(outputThread.native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }

  // Proof of concept display code
  int sd;
  struct sockaddr_un remote;

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  int sd_len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  while (state.running) {

    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      std::cerr << "failed to create display socket\n";
      exit(1);
    }

    if (connect(sd, (struct sockaddr *)&remote, sd_len) == -1) {
      std::cerr << "failed to connect to display socket\n";
      exit(1);
    }

    auto timeline = state.link.captureAppTimeline();
    const double tempo = timeline.tempo();
    char display_buf[16];
    sprintf(display_buf, "%.1f BPM\n", tempo);

    if (send(sd, display_buf, strlen(display_buf) + 1, 0) < 0) {
      std::cerr << "failed to send to display socket\n";
      exit(1);
    }

    close(sd);

    std::this_thread::sleep_for(std::chrono::milliseconds(17));
  }

  inputThread.join();
  outputThread.join();

  state.pins.UnexportAll();

  return 0;
}
