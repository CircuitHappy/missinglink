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

class UserInterface {

  public:

    enum class InputEvent {
      PLAY_STOP,
      TAP_TEMPO,
      ENC_DOWN,
      ENC_UP,
      ENC_PRESS
    };

    UserInterface();
    virtual ~UserInterface();

    void StartPollingInput();
    void StopPollingInput();

    // will be called on input polling thread
    std::function<void(InputEvent)> onInputEvent;

    void SetBPMModeLED(bool on);
    void SetLoopModeLED(bool on);
    void SetClockModeLED(bool on);

    // Index of animation LED starting from 0
    void SetAnimationLED(int index, bool on);

    void SetClock(bool on);
    void SetReset(bool on);

  private:

    std::unique_ptr<IOExpander> m_pExpander;

    std::unique_ptr<GPIO::Pin> m_pInterruptIn;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;

    std::atomic<bool> m_bStopPolling;
    std::unique_ptr<std::thread> m_pPollThread;

    unsigned int m_lastEncSeq;
    long m_encVal;

    void runPollInput();
    void handleInterrupt();
    void handleInputEvent(InputEvent event);
    void decodeEncoder(bool a_on, bool b_on);

};

} // namespace
