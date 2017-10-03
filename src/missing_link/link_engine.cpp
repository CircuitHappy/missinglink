/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "missing_link/common.h"
#include "missing_link/pin_defs.hpp"
#include "missing_link/gpio.hpp"
#include "missing_link/user_interface.hpp"
#include "missing_link/link_engine.hpp"

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

LinkEngine::State::State()
  : running(true)
  , playState(Playing)
  , link(120.0)
{
  link.enable(true);
}

LinkEngine::Process::Process(State &state)
  : m_state(state)
  , m_bStopped(true)
{}

LinkEngine::Process::~Process() {
  Stop();
}

void LinkEngine::Process::Run() {
  if (m_pThread != nullptr) { return; }
  m_bStopped = false;
  m_pThread = unique_ptr<thread>(new thread(&Process::run, this));
}

void LinkEngine::Process::Stop() {
  if (m_pThread == nullptr) { return; }
  m_bStopped = true;
  m_pThread->join();
  m_pThread = nullptr;
}

LinkEngine::UIProcess::UIProcess(State &state) : LinkEngine::Process(state) {}

void LinkEngine::UIProcess::run() {
  while (!m_bStopped) {
    this_thread::sleep_for(chrono::milliseconds(10));
  }
}


LinkEngine::LinkEngine()
  : m_pUI(shared_ptr<UserInterface>(new UserInterface()))
  , m_pUIProcess(unique_ptr<UIProcess>(new UIProcess(m_state)))
  , m_lastOutputTime(0)
{
  m_pUI->onInputEvent = [this](UserInterface::InputEvent event) {
    auto now = m_state.link.clock().micros();
    auto timeline = m_state.link.captureAppTimeline();
    auto tempo = timeline.tempo();
    switch (event) {
      case UserInterface::InputEvent::EncoderUp:
        tempo += 1.0;
        timeline.setTempo(tempo, now);
        break;
      case UserInterface::InputEvent::EncoderDown:
        tempo -= 1.0;
        timeline.setTempo(tempo, now);
        break;
      default:
        return;
    }
    m_state.link.commitAppTimeline(timeline);
  };
}

void LinkEngine::Run() {
  m_pUI->StartPollingInput();
  m_pUIProcess->Run();

  std::thread outputThread(&LinkEngine::runOutput, this);

  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(outputThread.native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }

  runDisplaySocket();

  m_pUI->StopPollingInput();
  m_pUIProcess->Stop();
  outputThread.join();
}

void LinkEngine::runOutput() {
  while (m_state.running) {
    const auto lastTime = m_lastOutputTime;
    const auto currentTime = m_state.link.clock().micros();

    m_lastOutputTime = currentTime;

    if (lastTime == chrono::microseconds(0) || currentTime < lastTime) {
      continue;
    }

    auto timeline = m_state.link.captureAudioTimeline();

    const double tempo = timeline.tempo();

    const double lastBeats = timeline.beatAtTime(lastTime, QUANTUM);
    const double currentBeats = timeline.beatAtTime(currentTime, QUANTUM);

    const double currentPhase = timeline.phaseAtTime(currentTime, QUANTUM);

    switch ((PlayState)m_state.playState) {
      case Playing: {
        const auto lastPulses = lastBeats * PULSES_PER_BEAT * 2.0;
        const auto currentPulses = currentBeats * PULSES_PER_BEAT * 2.0;

        const double secondsPerPhrase = 60.0 / (tempo / QUANTUM);
        const double resetHighFraction = PULSE_LENGTH / secondsPerPhrase;

        const bool resetHigh = (currentPhase <= resetHighFraction);
        m_pUI->SetReset(resetHigh);

        if (floor(currentPulses) > floor(lastPulses)) {
          const bool clockHigh = (int)(floor(currentPulses)) % 2 == 0;
          m_pUI->SetClock(clockHigh);
        }
        break;
      }
      default:
        m_pUI->SetClock(LOW);
        m_pUI->SetReset(LOW);
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
  strcpy(remote.sun_path, ML_DISPLAY_SOCK_PATH);
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

    auto timeline = m_state.link.captureAppTimeline();
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
