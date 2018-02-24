/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "missing_link/hw_defs.h"
#include "missing_link/gpio.hpp"
#include "missing_link/engine.hpp"

using namespace std;
using namespace MissingLink;
using namespace MissingLink::GPIO;

#define NUM_ANIM_FRAMES 4

namespace MissingLink {

  static const float CueAnimationFrames[][6] =  {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {1, 1, 0.1, 0.1, 0.1, 1},
    {1, 1, 1, 0.1, 1, 1},
    {1, 1, 1, 1, 1, 1}
  };

  static const float PlayAnimationFrames[][6] = {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {0.1, 1, 1, 0.1, 0.1, 0.1},
    {0.1, 0.1, 0.1, 1, 0.1, 0.1},
    {0.1, 0.1, 0.1, 0.1, 1, 1}
  };
}

Engine::State::State()
  : running(true)
  , playState(Stopped)
  , encoderMode(UserInterface::BPM)
  , settings(Settings::Load())
  , link(settings.load().tempo)
{
  link.enable(true);
}


Engine::Engine()
  : m_pUI(shared_ptr<UserInterface>(new UserInterface()))
  , m_pTapTempo(unique_ptr<TapTempo>(new TapTempo()))
  , m_lastOutputTime(0)
{
  m_pUI->onPlayStop = bind(&Engine::playStop, this);
  m_pUI->onTapTempo = bind(&TapTempo::Tap, m_pTapTempo.get());
  m_pUI->onEncoderRotate = bind(&Engine::routeEncoderAdjust, this, placeholders::_1);
  m_pUI->onEncoderPress = bind(&Engine::toggleMode, this);
  m_pUI->SetModeLED(m_state.encoderMode);
  m_pTapTempo->onNewTempo = bind(&Engine::setTempo, this, placeholders::_1);
}

void Engine::Run() {
  m_pUI->StartPollingInput();

  std::thread outputThread(&Engine::runOutput, this);
  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(outputThread.native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }

  std::thread displayThread(&Engine::runDisplayLoop, this);

  while (m_state.running) {
    Settings settings = m_state.settings.load();
    Settings::Save(settings);
    this_thread::sleep_for(chrono::seconds(1));
  }

  m_pUI->StopPollingInput();
  outputThread.join();
  displayThread.join();
}

void Engine::runOutput() {
  while (m_state.running) {
    const auto lastTime = m_lastOutputTime;
    const auto currentTime = m_state.link.clock().micros();

    m_lastOutputTime = currentTime;

    if (lastTime == chrono::microseconds(0) || currentTime < lastTime) {
      continue;
    }

    auto timeline = m_state.link.captureAudioTimeline();
    auto settings = m_state.settings.load();

    const double tempo = timeline.tempo();
    const double lastBeats = timeline.beatAtTime(lastTime, settings.quantum);
    const double currentBeats = timeline.beatAtTime(currentTime, settings.quantum);
    const double currentPhase = timeline.phaseAtTime(currentTime, settings.quantum);
    const double normalizedPhase = min(1.0, max(0.0, currentPhase / (double)settings.quantum));

    const int edgesPerBeat = settings.ppqn * 2;
    const int edgesPerLoop = edgesPerBeat * settings.quantum;
    const int lastEdges = (int)floor(lastBeats * (double)edgesPerBeat);
    const int currentEdges = (int)floor(currentBeats * (double)edgesPerBeat);
    const bool isNewEdge = currentEdges > lastEdges;

    const int animFrameIndex = min(NUM_ANIM_FRAMES - 1, max(0, (int)floor(normalizedPhase * NUM_ANIM_FRAMES)));

    switch ((PlayState)m_state.playState) {
      case Cued:
        //set state to playing if there are no peers or we are at the starting edge of loop
        if ( isNewEdge && currentEdges % edgesPerLoop == 0 ) {
          m_state.playState = Playing;
          // Deliberate fallthrough here
        } else {
          m_pUI->SetAnimationLEDs(CueAnimationFrames[animFrameIndex]);
          break;
        }
      case Playing: {
        const double secondsPerPhrase = 60.0 / (tempo / settings.quantum);
        const double resetHighFraction = PULSE_LENGTH / secondsPerPhrase;

        const bool resetHigh = (currentPhase <= resetHighFraction);
        m_pUI->SetReset(resetHigh);

        if (isNewEdge) {
          const bool clockHigh = currentEdges % 2 == 0;
          m_pUI->SetClock(clockHigh);
          m_pUI->SetAnimationLEDs(PlayAnimationFrames[animFrameIndex]);
        }

        break;
      }
      default:
        m_pUI->SetClock(LOW);
        m_pUI->SetReset(LOW);
        m_pUI->ClearAnimationLEDs();
        break;
    }

    this_thread::sleep_for(chrono::microseconds(500));
  }
}

void Engine::runDisplayLoop() {
  while (m_state.running) {
    auto value = formatDisplayValue();
    m_pUI->WriteDisplay(value);
    this_thread::sleep_for(chrono::milliseconds(50));
  }
}

std::string Engine::formatDisplayValue() {

  ostringstream stringStream;
  stringStream.setf(ios::fixed, ios::floatfield);
  stringStream.precision(1);

  switch (m_state.encoderMode) {
    case UserInterface::BPM: {
      auto timeline = m_state.link.captureAppTimeline();
      stringStream << timeline.tempo();
      break;
    }
    case UserInterface::LOOP:
      stringStream << (int)m_state.settings.load().quantum;
      break;
    case UserInterface::CLOCK:
      stringStream << (int)m_state.settings.load().ppqn;
      break;
    default:
      break;
  }

  return stringStream.str();
}

void Engine::playStop() {
  switch (m_state.playState) {
    case Stopped:
      // reset the timeline to zero if there are no peers
      if (m_state.link.numPeers() == 0) {
        resetTimeline();
      }
      m_state.playState = Cued;
      break;
    case Playing:
    case Cued:
      m_state.playState = Stopped;
      break;
    default:
      break;
  }
}

void Engine::toggleMode() {
  int mode = m_state.encoderMode;
  mode = (mode + 1) % (int)UserInterface::NUM_MODES;
  m_state.encoderMode = (UserInterface::EncoderMode)mode;
  m_pUI->SetModeLED((UserInterface::EncoderMode)mode);
}

void Engine::routeEncoderAdjust(float amount) {
  switch (m_state.encoderMode) {
    case UserInterface::BPM:
      tempoAdjust(amount);
      break;
    case UserInterface::LOOP:
      loopAdjust((int)amount);
      break;
    case UserInterface::CLOCK:
      ppqnAdjust((int)amount);
      break;
    default:
      break;
  }
}

void Engine::resetTimeline() {
  // Reset to beat zero in 1 ms
  auto timeline = m_state.link.captureAppTimeline();
  auto resetTime = m_state.link.clock().micros() + std::chrono::milliseconds(1);
  timeline.forceBeatAtTime(0, resetTime, m_state.settings.load().quantum);
  m_state.link.commitAppTimeline(timeline);
}

void Engine::tempoAdjust(float amount) {
  auto timeline = m_state.link.captureAppTimeline();
  double tempo = timeline.tempo() + amount;
  setTempo(tempo);
}

void Engine::loopAdjust(int amount) {
  auto settings = m_state.settings.load();
  settings.quantum = std::max(1, settings.quantum + amount);
  m_state.settings = settings;
}

void Engine::ppqnAdjust(int amount) {
  auto settings = m_state.settings.load();
  settings.ppqn = std::min(24, std::max(1, settings.ppqn + amount));
  m_state.settings = settings;
}

void Engine::setTempo(double tempo) {
  auto now = m_state.link.clock().micros();
  auto timeline = m_state.link.captureAppTimeline();
  timeline.setTempo(tempo, now);
  m_state.link.commitAppTimeline(timeline);

  auto settings = m_state.settings.load();
  settings.tempo = tempo;
  m_state.settings = settings;

  // switch back to tempo mode
  if (m_state.encoderMode != UserInterface::BPM) {
    m_state.encoderMode = UserInterface::BPM;
    m_pUI->SetModeLED(UserInterface::BPM);
  }
}
