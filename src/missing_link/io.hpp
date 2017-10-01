/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <string>
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class IO {

  public:

    enum InputEvent {
      PLAY_STOP,
      TAP_TEMPO,
      ENC_DOWN,
      ENC_UP,
      ENC_PRESS
    };

    IO();
    virtual ~IO();

    void StartPollingInput();
    void StopPollingInput();

    void SetBPMModeLED(bool on);
    void SetLoopModeLED(bool on);
    void SetClockModeLED(bool on);

//    void Display(const std::string &string);

    // will be called on input polling thread
    std::function<void(InputEvent)> f_inputEvent;

    // Below methods are NOT THREAD SAFE.
    // Must be called from single thread or use external mutex.

    void SetClock(bool on);
    void SetReset(bool on);

  private:

    std::mutex m_expanderMutex;
    std::unique_ptr<IOExpander> m_pExpander;

    std::unique_ptr<GPIO::Pin> m_pInterruptIn;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;

    std::atomic<bool> m_bStopPolling;
    std::unique_ptr<std::thread> m_pPollThread;

    void runPollInput();
    void handleInterrupt();

    void writeExpanderPin(IOExpander::Port port, int index, bool on);
};

} // namespace
