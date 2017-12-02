/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <thread>
#include <functional>
#include <string>
#include <vector>
#include "missing_link/gpio.hpp"
#include "missing_link/control.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class UserInterface {

  public:

    UserInterface();
    virtual ~UserInterface();

    void StartPollingInput();
    void StopPollingInput();

    // Outputs
    // These will be called from input polling thread
    std::function<void()> onPlayStop;
    std::function<void()> onTapTempo;
    std::function<void()> onEncoderPress;
    std::function<void(float)> onEncoderRotate;

    void SetBPMModeLED(bool on);
    void SetLoopModeLED(bool on);
    void SetClockModeLED(bool on);

    // Index of animation LED starting from 0
    void SetAnimationLED(int index, bool on);

    void SetClock(bool on);
    void SetReset(bool on);

  private:

    std::shared_ptr<IOExpander> m_pExpander;
    std::unique_ptr<ExpanderInputLoop> m_pInputLoop;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;
};

} // namespace
