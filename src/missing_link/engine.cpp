/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "missing_link/engine.hpp"
#include "missing_link/output.hpp"
#include "missing_link/user_interface.hpp"

#define MIN_TEMPO 20.0
#define MAX_TEMPO 300.0

using namespace std;
using namespace MissingLink;

Engine::Process::Process(Engine &engine, std::chrono::microseconds sleepTime)
  : m_engine(engine)
  , m_sleepTime(sleepTime)
  , m_bStopped(true)
{}

Engine::Process::~Process() {
  Stop();
}

void Engine::Process::Run() {
  if (m_pThread != nullptr) { return; }
  m_bStopped = false;
  m_pThread = unique_ptr<thread>(new thread(&Process::run, this));
}

void Engine::Process::Stop() {
  if (m_pThread == nullptr) { return; }
  m_bStopped = true;
  m_pThread->join();
  m_pThread = nullptr;
}

void Engine::Process::run() {
  while (!m_bStopped) {
    process();
    sleep();
  }
}

void Engine::Process::sleep() {
  std::this_thread::sleep_for(m_sleepTime);
}

Engine::Engine()
  : m_running(true)
  , m_playState(PlayState::Stopped)
  , m_wifiStatus(WifiState::NO_WIFI_FOUND)
  , m_pWifiStatusFile(unique_ptr<WifiStatus>(new WifiStatus()))
  , m_settings(Settings::Load())
  , m_inputMode(InputMode::BPM)
  , m_link(m_settings.load().tempo)
  , m_pView(shared_ptr<MainView>(new MainView()))
  , m_pTapTempo(unique_ptr<TapTempo>(new TapTempo()))
{
  Settings settings = m_settings.load();

  m_link.enable(true);

  m_link.enableStartStopSync(settings.start_stop_sync);

  auto outputProcess = unique_ptr<OutputProcess>(new OutputProcess(*this));
  m_processes.push_back(std::move(outputProcess));

  auto viewProcess = unique_ptr<ViewUpdateProcess>(new ViewUpdateProcess(*this, m_pView));
  m_processes.push_back(std::move(viewProcess));

  auto uiProcess = unique_ptr<UserInputProcess>(new UserInputProcess(*this));
  uiProcess->onPlayStop = bind(&Engine::playStop, this);
  uiProcess->onResetGesture = bind(&Engine::resetAtLoopStart, this);
  uiProcess->onTapTempo = bind(&TapTempo::Tap, m_pTapTempo.get());
  uiProcess->onEncoderRotate = bind(&Engine::routeEncoderAdjust, this, placeholders::_1);
  uiProcess->onEncoderPress = bind(&Engine::toggleMode, this);
  m_processes.push_back(std::move(uiProcess));

  m_pTapTempo->onNewTempo = bind(&Engine::setTempo, this, placeholders::_1);

  m_link.setNumPeersCallback([this](std::size_t numPeers) {
    std::string message = "    " + std::to_string(numPeers) + " LINKS    ";
    m_pView->WriteDisplayTemporarily(message, 2000, true);
  });

  m_link.setTempoCallback([this](const double tempo) {
    if (m_inputMode == InputMode::BPM) {
      displayTempo(tempo, false);
    }
  });

  m_link.setStartStopCallback([this](const bool isPlaying) {
    std::string message;
    const auto timeline = m_link.captureAppSessionState();
    if (timeline.isPlaying()) {
      m_playState = PlayState::Cued;
      //message = "    SYNC START    "; //would be nice to display status if remotely start/stop
    } else {
      m_playState = PlayState::Stopped;
      //message = "    SYNC STOP    "; //but these display even if local play button is hit
    }
    //m_pView->WriteDisplayTemporarily(message, 2000, true);
  });

}

void Engine::Run() {
  WifiState prevWifiStatus;
  displayTempo(getCurrentTempo(), true);

  for (auto &process : m_processes) {
    process->Run();
  }

  while (isRunning()) {
    Settings settings = m_settings.load();
    Settings::Save(settings);
    prevWifiStatus = m_wifiStatus;
    m_wifiStatus = m_pWifiStatusFile->ReadStatus();
    if (prevWifiStatus != m_wifiStatus) displayTempWifiStatus(m_wifiStatus);
    this_thread::sleep_for(chrono::seconds(1));
  }

  for (auto &process : m_processes) {
    process->Stop();
  }
}

const double Engine::GetNormalizedPhase() const {
  const auto now = m_link.clock().micros() + std::chrono::milliseconds(getCurrentDelayCompensation());
  const auto currentSettings = m_settings.load();
  const auto timeline = m_link.captureAppSessionState();
  const double phase = timeline.phaseAtTime(now, currentSettings.quantum);
  return min(1.0, max(0.0, phase / (double)currentSettings.quantum));
}

const Engine::OutputModel Engine::GetOutputModel(std::chrono::microseconds last) const {
  OutputModel output;

  const auto now = m_link.clock().micros() - std::chrono::milliseconds(getCurrentDelayCompensation());
  output.now = now;

  auto timeline = m_link.captureAudioSessionState();
  output.tempo = timeline.tempo();

  if (last == std::chrono::microseconds(0)) {
    output.clockTriggered = false;
    output.resetTriggered = false;
    return output;
  }

  const auto currentSettings = m_settings.load();
  const double beats = timeline.beatAtTime(now, currentSettings.quantum);
  const double lastBeats = timeline.beatAtTime(last, currentSettings.quantum);

  const int edgesPerBeat = currentSettings.getPPQN() * 2;
  const int edgesPerLoop = edgesPerBeat * currentSettings.quantum;

  const int edge = (int)floor(beats * (double)edgesPerBeat);
  const int lastEdge = (int)floor(lastBeats * (double)edgesPerBeat);

  output.clockTriggered = (edge % 2 == 0) && (edge != lastEdge);
  output.resetTriggered = output.clockTriggered && (edge % edgesPerLoop == 0);

  return output;
}

void Engine::SetPlayState(PlayState state) {
  m_playState = state;
  if (state == PlayState::Stopped) {
    stopTimeline();
  }
}

int Engine::getWifiStatus() {
  return m_wifiStatus;
}

int Engine::getResetMode() {
  return getCurrentResetMode();
}

void Engine::playStop() {
  switch (m_playState) {
    case PlayState::Stopped:
      startTimeline();
      m_playState = PlayState::Cued;
      break;
    case PlayState::Playing:
      m_playState = PlayState::CuedStop;
      break;
    case PlayState::Cued:
    case PlayState::CuedStop:
      m_playState = PlayState::Stopped;
      stopTimeline();
      break;
    default:
      break;
  }
}

void Engine::resetAtLoopStart() {
  m_pView->WriteDisplayTemporarily("    RESET AT LOOP START    ", 5000, true);
}

void Engine::toggleMode() {
  auto now = Clock::now();
  // Only switch to next mode if toggle pressed twice within 1.5 seconds
  if (now - m_lastToggle < std::chrono::milliseconds(1500)) {
    m_inputMode = static_cast<InputMode>((static_cast<int>(m_inputMode.load()) + 1) % 6);
  }
  m_lastToggle = Clock::now();
  displayCurrentMode();
}

void Engine::startTimeline() {
  auto timeline = m_link.captureAppSessionState();
  auto now = m_link.clock().micros();
  if (m_link.numPeers() == 0){
    timeline.forceBeatAtTime(0, now + std::chrono::milliseconds(1), m_settings.load().quantum);
    timeline.setIsPlaying(true, now + std::chrono::milliseconds(1));
  } else {
    timeline.setIsPlayingAndRequestBeatAtTime(true, now, 0, m_settings.load().quantum);
  }
  m_link.commitAppSessionState(timeline);
}

void Engine::stopTimeline() {
  auto timeline = m_link.captureAppSessionState();
  auto now = m_link.clock().micros();
  timeline.setIsPlayingAndRequestBeatAtTime(false, now, 0, m_settings.load().quantum);
  m_link.commitAppSessionState(timeline);
}

void Engine::setTempo(double tempo) {

  tempo = std::max(MIN_TEMPO, std::min(MAX_TEMPO, tempo));

  auto now = m_link.clock().micros();
  auto timeline = m_link.captureAppSessionState();
  timeline.setTempo(tempo, now);
  m_link.commitAppSessionState(timeline);

  auto settings = m_settings.load();
  settings.tempo = tempo;
  m_settings = settings;

  // switch back to tempo mode
  m_inputMode = InputMode::BPM;
  displayTempo(tempo, true);
}

void Engine::routeEncoderAdjust(float amount) {
  switch (m_inputMode) {
    case InputMode::BPM:
      tempoAdjust(round(amount));
      break;
    case InputMode::Loop:
      loopAdjust(amount > 0.0 ? 1 : -1);
      break;
    case InputMode::Clock:
      ppqnAdjust(amount > 0.0 ? 1 : -1);
      break;
    case InputMode::ResetMode:
      resetModeAdjust(amount > 0.0 ? 1 : -1);
      break;
    case InputMode::DelayCompensation:
      delayCompensationAdjust(amount);
      break;
    case InputMode::StartStopSync:
      StartStopSyncAdjust(amount);
      break;
    default:
      break;
  }
}

void Engine::tempoAdjust(float amount) {
  setTempo(getCurrentTempo() + amount);
}

void Engine::loopAdjust(int amount) {
  auto settings = m_settings.load();
  int quantum = std::max(1, settings.quantum + amount);
  settings.quantum = quantum;
  m_settings = settings;
  displayQuantum(quantum, true);
}

void Engine::delayCompensationAdjust(int amount) {
  auto settings = m_settings.load();
  int delay = settings.delay_compensation + amount;
  settings.delay_compensation = delay;
  m_settings = settings;
  displayDelayCompensation(delay, true);
}

void Engine::ppqnAdjust(int amount) {
  int max_index = Settings::ppqn_options.size() - 1;
  auto settings = m_settings.load();
  int index = std::min(max_index, std::max(0, settings.ppqn_index + amount));
  settings.ppqn_index = index;
  m_settings = settings;
  int ppqn = Settings::ppqn_options[index];
  displayPPQN(ppqn, true);
}

void Engine::resetModeAdjust(int amount) {
  int num_options = 3;
  auto settings = m_settings.load();
  int mode = std::min(num_options - 1, std::max(0, settings.reset_mode + amount));
  if ( (mode > 0) && (getCurrentPPQN() != 24) ) {
    //set clock to 24 PPQN
    settings.ppqn_index = 6;
    m_pView->WriteDisplayTemporarily("    CLOCK NOW 24 PPQN    ", 3000, true);
  }
  settings.reset_mode = mode;
  m_settings = settings;
  displayResetMode(mode, true);
}

void Engine::StartStopSyncAdjust(float amount) {
  //clockwise set value to true, counterclock set value to false
  auto settings = m_settings.load();
  bool ss_sync = settings.start_stop_sync;
  ss_sync = amount > 0 ? true : false;
  settings.start_stop_sync = ss_sync;
  m_settings = settings;
  m_link.enableStartStopSync(ss_sync);
  displayStartStopSync(ss_sync, true);
}

void Engine::displayCurrentMode() {
  const int holdTime = 1500;
  switch (m_inputMode) {
    case InputMode::BPM: {
      m_pView->WriteDisplayTemporarily("BPM", holdTime);
      displayTempo(getCurrentTempo(), false);
      break;
    }
    case InputMode::Loop: {
      m_pView->WriteDisplayTemporarily("LOOP", holdTime);
      displayQuantum(getCurrentQuantum(), false);
      break;
    }
    case InputMode::Clock: {
      m_pView->WriteDisplayTemporarily("PPQN", holdTime);
      displayPPQN(getCurrentPPQN(), false);
      break;
    }
    case InputMode::ResetMode: {
      m_pView->WriteDisplayTemporarily("    RESET MODE    ", 2400, true);
      displayResetMode(getCurrentResetMode(), false);
      break;
    }
    case InputMode::DelayCompensation: {
      m_pView->WriteDisplayTemporarily("    OFFSET (MS)    ", 2200, true);
      displayDelayCompensation(getCurrentDelayCompensation(), false);
      break;
    }
    case InputMode::StartStopSync: {
      m_pView->WriteDisplayTemporarily("    SYNC START/STOP    ", 3000, true);
      displayStartStopSync(getCurrentStartStopSync(), false);
      break;
    }
    default:
      break;
  }
}

void Engine::displayTempWifiStatus(WifiState status) {
  const int oneSecond = 1000;
  switch (status) {
    case AP_MODE :
      m_pView->WriteDisplayTemporarily("    ACCESS POINT MODE    ", oneSecond * 300, true);
    break;
    case TRYING_TO_CONNECT :
      m_pView->WriteDisplayTemporarily("    SEARCHING FOR WIFI    ", oneSecond * 60, true);
      break;
    case NO_WIFI_FOUND :
      m_pView->WriteDisplayTemporarily("    NO WIFI FOUND    ", oneSecond * 10, true);
      break;
    case WIFI_CONNECTED :
      m_pView->WriteDisplayTemporarily("    WIFI CONNECTED    ", oneSecond * 5, true);
      break;
      case REBOOT :
        m_pView->WriteDisplayTemporarily("BOOT", oneSecond * 10, false);
        break;
    default :
      break;
  }
}

void Engine::displayTempo(double tempo, bool force) {
  std::ostringstream stringStream;
  stringStream.setf(std::ios::fixed, std::ios::floatfield);
  stringStream.precision(1);
  stringStream << tempo;
  m_pView->WriteDisplay(stringStream.str(), force);
}

void Engine::displayQuantum(int quantum, bool force) {
  m_pView->WriteDisplay(std::to_string(quantum), force);
}

void Engine::displayPPQN(int ppqn, bool force) {
  m_pView->WriteDisplay(std::to_string(ppqn), force);
}

void Engine::displayResetMode(int mode, bool force) {
  switch (mode) {
    case 0:
      m_pView->WriteDisplay("PULS", force);
      break;
    case 1:
      m_pView->WriteDisplay("DIN1", force);
      break;
    case 2:
      m_pView->WriteDisplay("DIN2", force);
      break;
    default:
      m_pView->WriteDisplay("WHAT", force);
      break;
  }
}

void Engine::displayDelayCompensation(int delay, bool force) {
  m_pView->WriteDisplay(std::to_string(delay), force);
}

void Engine::displayStartStopSync(bool sync, bool force) {
  if (sync == true) {
    m_pView->WriteDisplay("ON", force);
  } else {
    m_pView->WriteDisplay("OFF", force);
  }
}

double Engine::getCurrentTempo() const {
  auto timeline = m_link.captureAppSessionState();
  return timeline.tempo();
}

int Engine::getCurrentQuantum() const {
  auto settings = m_settings.load();
  return settings.quantum;
}

int Engine::getCurrentPPQN() const {
  auto settings = m_settings.load();
  return Settings::ppqn_options[settings.ppqn_index];
}

int Engine::getCurrentResetMode() const {
  auto settings = m_settings.load();
  return settings.reset_mode;
}

int Engine::getCurrentDelayCompensation() const {
  auto settings = m_settings.load();
  return settings.delay_compensation;
}

int Engine::getCurrentStartStopSync() const {
  auto settings = m_settings.load();
  return settings.start_stop_sync;
}
