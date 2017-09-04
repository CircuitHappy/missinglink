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

LinkEngine::LinkEngine()
  : m_link(120.0)
  , m_clockOut(GPIO::CHIP_D0, GPIO::Pin::OUT)
  , m_resetOut(GPIO::CHIP_D1, GPIO::Pin::OUT)
  , m_playingOut(GPIO::CHIP_D6, GPIO::Pin::OUT)
  , m_playStopIn(GPIO::CHIP_D7, GPIO::Pin::IN)
  , m_btnPlayStop(m_playStopIn)
  , m_lastOutputTime(0)
{
  m_clockOut.Export();
  m_resetOut.Export();
  m_playingOut.Export();
  m_playStopIn.Export();
  m_link.enable(true);
}

LinkEngine::State::State()
  : running(true)
  , playState(Stopped)
{}

void LinkEngine::Run() {
  std::thread inputThread(&LinkEngine::runInput, this);
  std::thread outputThread(&LinkEngine::runOutput, this);

  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(outputThread.native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }

  runDisplaySocket();
  inputThread.join();
  outputThread.join();
}

void LinkEngine::runInput() {

  while (m_state.running) {

    m_btnPlayStop.Process();

    if (m_btnPlayStop.GetCurrentEvent() == GPIO::Button::TOUCH_DOWN) {
      switch ((PlayState)m_state.playState) {
        case Stopped:
          m_state.playState.store(Cued);
          break;
        case Cued:
        case Playing:
          m_state.playState.store(Stopped);
          break;
      }
    }

    this_thread::sleep_for(chrono::milliseconds(10));
  }

}

void LinkEngine::runOutput() {
  while (m_state.running) {
    const auto lastTime = m_lastOutputTime;
    const auto currentTime = m_link.clock().micros();

    m_lastOutputTime = currentTime;

    if (lastTime == chrono::microseconds(0) || currentTime < lastTime) {
      continue;
    }

    auto timeline = m_link.captureAudioTimeline();

    const double tempo = timeline.tempo();

    const double lastBeats = timeline.beatAtTime(lastTime, QUANTUM);
    const double currentBeats = timeline.beatAtTime(currentTime, QUANTUM);

    const double lastPhase = timeline.phaseAtTime(lastTime, QUANTUM);
    const double currentPhase = timeline.phaseAtTime(currentTime, QUANTUM);

    switch ((PlayState)m_state.playState) {
      case Cued: {
        const bool playHigh = (long)(currentBeats * 2) % 2 == 0;
        m_playingOut.Write(playHigh ? HIGH : LOW);
        if (currentPhase < lastPhase) {
          m_state.playState.store(Playing);
        }
        break;
      }
      case Playing: {
        m_playingOut.Write(HIGH);

        const auto lastPulses = lastBeats * PULSES_PER_BEAT * 2.0;
        const auto currentPulses = currentBeats * PULSES_PER_BEAT * 2.0;

        const double secondsPerPhrase = 60.0 / (tempo / QUANTUM);
        const double resetHighFraction = PULSE_LENGTH / secondsPerPhrase;

        const bool resetHigh = (currentPhase <= resetHighFraction);
        m_resetOut.Write(resetHigh ? HIGH : LOW);

        if (floor(currentPulses) > floor(lastPulses)) {
          const bool clockHigh = (int)(floor(currentPulses)) % 2 == 0;
          m_clockOut.Write(clockHigh ? HIGH : LOW);
        }
        break;
      }
      default:
        m_playingOut.Write(LOW);
        m_clockOut.Write(LOW);
        m_resetOut.Write(LOW);
        break;
    }

    this_thread::sleep_for(chrono::microseconds(500));
  }
}

void LinkEngine::runDisplaySocket() {
  // Proof of concept display code
  int sd;
  struct sockaddr_un remote;

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  int sd_len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  while (m_state.running) {

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

    auto timeline = m_link.captureAppTimeline();
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
