/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <algorithm>
#include <thread>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ableton/Link.hpp>
#include "missing_link/hw_defs.h"
#include "missing_link/types.hpp"
#include "missing_link/view.hpp"
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"
#include "missing_link/wifi_status.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;
using std::min;
using std::max;

OutputProcess::OutputProcess(Engine &engine)
  : Engine::Process(engine, std::chrono::seconds(1), std::chrono::microseconds(SAMPLE_SIZE))
  , m_lastBeat(0.0)
  , m_pOutA(std::unique_ptr<OutputJack>(new OutputJack(&engine, ML_CLOCK_PIN, OutputJack::CLOCK_NORMAL, 4)))
  , m_pOutB(std::unique_ptr<OutputJack>(new OutputJack(&engine, ML_RESET_PIN, OutputJack::RESET_TRIGGER, 2)))
{

}

void OutputProcess::Run() {
  Process::Run();
  sched_param param;
  param.sched_priority = 90;
  if(::pthread_setschedparam(m_pThread->native_handle(), SCHED_FIFO, &param) < 0) {
    std::cerr << "Failed to set output thread priority\n";
  }
}

void OutputProcess::process() {
    double quantum = 4.0;
    auto sessionState = m_engine.getAudioSessionState();
    std::chrono::microseconds hostTime = m_engine.getDelayCompNow();
    // auto beginHostTime = m_pEngine->getDelayCompNow() + std::chrono::microseconds(BUFFER_LENGTH * SAMPLE_SIZE);
    // std::chrono::microseconds lastTime = beginHostTime - std::chrono::microseconds(SAMPLE_SIZE);
    auto playState = m_engine.GetPlayState();

    bool resetTriggered = false;

    // const auto hostTime = beginHostTime + std::chrono::microseconds(llround(i * SAMPLE_SIZE)); //maybe not llround
    const double beats = sessionState.phaseAtTime(hostTime, quantum);
    // const double beats = sessionState.beatAtTime(hostTime, quantum);

    // m_pOutA->writeToJack(((int)beats % 4) == 0);
    // m_pOutB->writeToJack((int)phase < 1.0);

    const int edgesPerBeat = 24; //24 ppqn, times 2
    const int edgesPerLoop = edgesPerBeat * quantum;
    const int edge = (int)floor(beats * (double)edgesPerBeat);
    const int lastEdge = (int)floor(m_lastBeat * (double)edgesPerBeat);

    resetTriggered = ((edge % 2 == 0) && (edge != lastEdge)) && (edge % edgesPerLoop == 0);

    switch (playState)
    {
      case Engine::PlayState::Stopped:
        if (m_transportStopped == false) {
          m_transportStopped = true;
        }
        break;

      case Engine::PlayState::Cued:
        // start playing on first clock of loop
        if (resetTriggered == false) {
          break;
        }
        // Deliberate fallthrough here
        m_engine.startTimeline(hostTime);
        m_engine.SetPlayState(Engine::PlayState::Playing);
        playState = Engine::PlayState::Playing;

      case Engine::PlayState::Playing:
        break;

      case Engine::PlayState::CuedStop:
        // stop playing at start of next loop
        if (resetTriggered) {
          m_engine.SetPlayState(Engine::PlayState::Stopped);
          playState = Engine::PlayState::Stopped;
        }
        break;

      default:
        break;
    }

    m_pOutA->setJack(beats, resetTriggered, playState);
    m_pOutB->setJack(beats, resetTriggered, playState);

    m_lastTime = hostTime;
    m_lastBeat = beats;
}

OutputJack::OutputJack(Engine * pEngine, uint8_t pin, output_mode_t mode, uint8_t ppqn)
  : m_pEngine(pEngine)
  , m_trigHighCount(0)
  , m_pGpioPin(std::unique_ptr<Pin>(new Pin(pin, Pin::OUT)))
  , m_outMode(mode)
  , m_ppqn(ppqn)
  , m_currentState(false)
  , m_lastCalcNum(-1)
  , m_sampleCount(0)
{

}

void OutputJack::setJack(const double beat, const bool resetTrig, const Engine::PlayState playState)
{
  bool clockTriggered = false;
  bool high = false;

  if (m_trigHighCount > 0) {
    m_trigHighCount --;
    high = true;
  } else {
    switch (m_outMode)
    {
      case CLOCK_NORMAL:
        if ((playState == Engine::PlayState::Playing) || (playState == Engine::PlayState::CuedStop))
        {
          high = calcClockTrig(beat);
          clockTriggered = high;
        }
        break;
      case CLOCK_ALWAYS_ON:
        high = calcClockTrig(beat);
        clockTriggered = high;
        break;
      case RESET_TRIGGER:
        if ((playState == Engine::PlayState::Playing) || (playState == Engine::PlayState::CuedStop))
        {
          high = resetTrig;
          clockTriggered = high;
        }
        break;
      case RUN_HIGH:
        if ((playState == Engine::PlayState::Playing) || (playState == Engine::PlayState::CuedStop))
        {
          high = true;
          //not a trigger so we do not need to set clockTriggered
        }
        break;
      default:
        high = false;
        break;
    }
    if (clockTriggered) {
      m_trigHighCount = (TRIG_LENGTH / SAMPLE_SIZE) - 1;
    }
  }
  writeToJack(high);
}

bool OutputJack::calcClockTrig(double beat)
{
  double wholeNum;
  double clockPhase = modf(beat, &wholeNum) * m_ppqn;
  int currentClockNum = floor(clockPhase);
  bool trig = (currentClockNum != m_lastCalcNum) ? true : false;
  m_lastCalcNum = currentClockNum;
  return trig;
}

bool OutputJack::calcResetTrig(double beat, double quantum)
{
  uint8_t currentBeatNum = (int)beat % (int)quantum;
  bool trig = ( (currentBeatNum == 0) && (currentBeatNum != m_lastCalcNum) );
  m_lastCalcNum = currentBeatNum;
  return trig;
}

void OutputJack::writeToJack(bool state)
{
  m_currentState = state;
  m_pGpioPin->Write((DigitalValue)state);
}

namespace MissingLink {

  static const int NUM_ANIM_FRAMES = 6;

  static const float CueAnimationFrames[][6] =  {
    {0.2, 0, 0, 0.1, 0.2, 0.3},
    {0.2, 0.2, 0, 0, 0.1, 0.2},
    {0.2, 0.2, 0.2, 0, 0, 0.1},
    {0.2, 0.2, 0.2, 0.2, 0, 0},
    {0.2, 0.2, 0.2, 0.2, 0.2, 0},
    {0.2, 0.2, 0.2, 0.2, 0.2, 0.2},
  };

  static const float PlayAnimationFrames[][6] = {
    {1, 0.1, 0.1, 0.1, 0.1, 0.1},
    {0.1, 1, 0.1, 0.1, 0.1, 0.1},
    {0.1, 0.1, 1, 0.1, 0.1, 0.1},
    {0.1, 0.1, 0.1, 1, 0.1, 0.1},
    {0.1, 0.1, 0.1, 0.1, 1, 0.1},
    {0.1, 0.1, 0.1, 0.1, 0.1, 1},
  };

  static const float CuedStopAnimationFrames[][6] = {
    {0, 0.1, 0.2, 0.3, 0.4, 0.5},
    {0, 0.03, 0.1, 0.2, 0.3, 0.4},
    {0, 0, 0.03, 0.1, 0.2, 0.3},
    {0, 0, 0, 0.03, 0.1, 0.2},
    {0, 0, 0, 0, 0.03, 0.1},
    {0.01, 0, 0, 0, 0, 0},
  };

  static const float StoppedAnimationFrames[][6] = {
    {0.05, 0, 0, 0.02, 0.02, 0.03},
    {0.03, 0.05, 0, 0, 0.02, 0.02},
    {0.02, 0.03, 0.05, 0, 0, 0.02},
    {0.02, 0.02, 0.03, 0.05, 0, 0},
    {0, 0.02, 0.02, 0.03, 0.05, 0},
    {0, 0, 0.02, 0.02, 0.03, 0.05},
  };

  //Animation for WIFI LED when AP is ready to connect to
  static std::vector<float> WifiAccessPointReady =
    {
      0, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55,
      0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
      0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55, 0.5, 0.45, 0.4,
      0.35, 0.3, 0.25, 0.2, 0.15, 0.1, 0.05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

  //Animation for WIFI LED when failed to connect to known AP, booting AP mode
  static std::vector<float> WifiTryingToConnect =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

}

ViewUpdateProcess::ViewUpdateProcess(Engine &engine, std::shared_ptr<MainView> pView)
  : Engine::Process(engine, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(15)), std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(15)))
  , m_pView(pView)
{}

void ViewUpdateProcess::process() {
  const auto phase = m_engine.GetNormalizedPhase();
  const double beatPhase = m_engine.GetBeatPhase();
  const auto playState = m_engine.GetPlayState();
  m_pView->setLogoLight(beatPhase);
  animatePhase(phase, playState);
  m_pView->displayWifiStatusFrame(getWifiStatusFrame(m_engine.getWifiStatus()));
  m_pView->ScrollTempMessage();
  m_pView->UpdateDisplay();
}

void ViewUpdateProcess::animatePhase(float normalizedPhase, Engine::PlayState playState) {

  const int animFrameIndex = min(
    NUM_ANIM_FRAMES - 1,
    max(0, (int)floor(normalizedPhase * NUM_ANIM_FRAMES))
  );

  switch (playState) {
    case Engine::PlayState::Cued:
      m_pView->SetAnimationLEDs(CueAnimationFrames[animFrameIndex]);
      break;
    case Engine::PlayState::Playing:
      m_pView->SetAnimationLEDs(PlayAnimationFrames[animFrameIndex]);
      break;
    case Engine::PlayState::CuedStop:
      m_pView->SetAnimationLEDs(CuedStopAnimationFrames[animFrameIndex]);
      break;
    case Engine::PlayState::Stopped:
        if (m_engine.GetNumberOfPeers() > 0) {
          m_pView->SetAnimationLEDs(StoppedAnimationFrames[animFrameIndex]);
        } else {
          m_pView->ClearAnimationLEDs();
        }
        break;
    default:
      m_pView->ClearAnimationLEDs();
      break;
  }
}

float ViewUpdateProcess::getWifiStatusFrame(int wifiStatus) {
  std::vector<float> animationFrames;
  static int frameCount = 1;
  static int frameIndex = 0;
  //int wifiStatus = AP_MODE;
  switch (wifiStatus) {
    case AP_MODE :
      animationFrames = WifiAccessPointReady;
    break;
    case TRYING_TO_CONNECT :
      animationFrames = WifiTryingToConnect;
      break;
    case NO_WIFI_FOUND :
      animationFrames = {0.0};
      break;
    case WIFI_CONNECTED :
      animationFrames = {1.0};
      break;
    case REBOOT :
      animationFrames = {0.0};
      break;
    default :
      animationFrames = {0.0};
      break;
  }
  frameCount = animationFrames.size();
  frameIndex++;
  if (frameIndex > (frameCount - 1)) {
    frameIndex = 0;
  }
  return animationFrames.at(frameIndex);
}
