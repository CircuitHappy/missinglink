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
#include "missing_link/output_jack.hpp"
#include "missing_link/wifi_status.hpp"

using namespace MissingLink;
using namespace MissingLink::GPIO;
using namespace std;
using std::min;
using std::max;

OutputProcess::OutputProcess(Engine &engine, std::shared_ptr<OutputJack> pJackA, std::shared_ptr<OutputJack> pJackB)
  : Engine::Process(engine, std::chrono::seconds(1), std::chrono::microseconds(100))
  , m_pJackA(pJackA)
  , m_pJackB(pJackB)
  , m_pClockOut(std::unique_ptr<Pin>(new Pin(ML_CLOCK_PIN, Pin::OUT)))
  , m_pResetOut(std::unique_ptr<Pin>(new Pin(ML_RESET_PIN, Pin::OUT)))
{
  m_pClockOut->Write(LOW);
  m_pResetOut->Write(LOW);
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
  auto midiOut = m_engine.GetMidiOut();
  auto playState = m_engine.GetPlayState();
  auto mainView = m_engine.GetMainView();
  const auto model = m_engine.GetOutputModel(m_lastOutTime);
  OutputProcess::OutputMode outAMode = intToOutputMode(m_engine.GetOutAMode());
  OutputProcess::OutputMode outBMode = intToOutputMode(m_engine.GetOutBMode());
  int outAPPQN = m_engine.GetOutAPPQN();
  int outBPPQN = m_engine.GetOutBPPQN();
  // std::cout << "clockDiv: " << 48 / outAPPQN << "; outAPPQN: " << outAPPQN << std::endl;
  m_lastOutTime = model.now;
  if (model.clockNum == 0)
  {
    switch (playState)
    {
      case Engine::PlayState::Cued:
        m_engine.SetPlayState(Engine::PlayState::Playing);
        playState = Engine::PlayState::Playing;
        midiOut->StartTransport();
        mainView->flashLedRing();
        m_transportStopped = false;
        break;
      case Engine::PlayState::CuedStop:
        m_engine.SetPlayState(Engine::PlayState::Stopped);
        playState = Engine::PlayState::Stopped;
        if (m_transportStopped == false) {
          midiOut->StopTransport();
          m_transportStopped = true;
        }
        break;
      default:
        break;
    }
  }

  bool outAState = GetOutState(model.clockTriggered, model.clockNum, playState, 4, outAMode);
  bool outBState = GetOutState(model.clockTriggered, model.clockNum, playState, 2, outBMode);

  if (model.clockTriggered) { 
    // setOutA(outAState);
    // setOutB(outBState);
    m_pJackA->RequestTrigger();
    if (model.clockNum == 0) m_pJackB->RequestTrigger();
    // if (outBState) m_pJackB->RequestTrigger();
    midiOut->ClockOut();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    // if (outAMode != OutputProcess::OutputMode::GATE_MODE) {
    //   setOutA(false);
    // }
    // if (outBMode != OutputProcess::OutputMode::GATE_MODE) {
    //     setOutB(false);
    // }
  }
}

bool OutputProcess::GetOutState(bool triggered, uint16_t clockNum, Engine::PlayState playState, int ppqn, OutputProcess::OutputMode mode)
{
  uint16_t clockDiv = 48 / (uint16_t)ppqn;
  if (clockDiv < 1) clockDiv = 1;
  switch (mode)
  {
  case OutputProcess::OutputMode::CLOCK_ALWAYS_ON_MODE:
    if (triggered && ((clockNum % clockDiv) == 0))
    {
      return true;
    }
    break;
  case OutputProcess::OutputMode::CLOCK_NORMAL_MODE:
    if ((playState == Engine::PlayState::Playing) || (playState == Engine::PlayState::CuedStop))
    {
      if (triggered && ((clockNum % clockDiv) == 0))
      {
        return true;
      }
    }
    break;
  case OutputProcess::OutputMode::RESET_TRIGGER_MODE:
    if ((playState == Engine::PlayState::Playing) || (playState == Engine::PlayState::CuedStop))
    {
      if (triggered && (clockNum == 0))
      {
        return true;
      }
    }
    break;
  case OutputProcess::OutputMode::GATE_MODE:
    if ((playState == Engine::PlayState::Playing) || (playState == Engine::PlayState::CuedStop))
    {
      return true;
    }
    else
    {
      return false;
    }
    break;
  }
  return false;
}

void OutputProcess::setOutA(bool high) {
  if (m_clockHigh == high) { return; }
  m_clockHigh = high;
  m_pClockOut->Write(high ? HIGH : LOW);
}

void OutputProcess::setOutB(bool high) {
  if (m_resetHigh == high) { return; }
  m_resetHigh = high;
  m_pResetOut->Write(high ? HIGH : LOW);
}

OutputProcess::OutputMode OutputProcess::intToOutputMode(uint8_t mode)
{
  return static_cast<OutputProcess::OutputMode>(mode);
}

int OutputProcess::OutputModeToInt(OutputProcess::OutputMode mode)
{
  return static_cast<int>(mode);
}

uint16_t OutputProcess::GetPpqnValue(uint16_t index)
{
  uint8_t ppqn;
  OutputProcess::PpqnItem item = static_cast<OutputProcess::PpqnItem>(index);
  switch (item)
  {
  case OutputProcess::PpqnItem::PPQN_1:
    ppqn = 1;
    break;
  case OutputProcess::PpqnItem::PPQN_2:
    ppqn = 2;
    break;
  case OutputProcess::PpqnItem::PPQN_4:
    ppqn = 4;
    break;
  case OutputProcess::PpqnItem::PPQN_8:
    ppqn = 8;
    break;
  case OutputProcess::PpqnItem::PPQN_12:
    ppqn = 12;
    break;
  case OutputProcess::PpqnItem::PPQN_24:
    ppqn = 24;
    break;
  default:
    ppqn = 1;
    break;
  }
  return ppqn;
}

uint16_t OutputProcess::GetPpqnItemIndex(uint8_t ppqnValue)
{
  uint16_t index;
  switch (ppqnValue)
  {
  case 1:
    index = 0;
    break;
  case 2:
    index = 1;
    break;
  case 4:
    index = 2;
    break;
  case 8:
    index = 3;
    break;
  case 12:
    index = 4;
    break;
  case 24:
    index = 5;
    break;
  default:
    index = 0;
    break;
  }
  return index;
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

JackProcess::JackProcess(Engine &engine, std::shared_ptr<OutputJack> pJack)
:  Engine::Process(engine, std::chrono::seconds(1), std::chrono::microseconds(100))
, m_pJack(pJack)
{
  
}

void JackProcess::Run() {
  Process::Run();
  // sched_param param;
  // param.sched_priority = 50;
  // if(::pthread_setschedparam(m_pThread->native_handle(), SCHED_FIFO, &param) < 0) {
  //   std::cerr << "Failed to set output thread priority\n";
  // }
}

void JackProcess::process()
{
  m_run = true;
  while (m_run == true)
  {
    while (m_pJack->TriggerRequested() == false)
    {
      this_thread::yield();
    }
    m_pJack->Write(true);
    this_thread::sleep_for(chrono::milliseconds(5));
    m_pJack->Write(false);
  }
}