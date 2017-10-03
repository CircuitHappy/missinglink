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
#include <vector>
#include "missing_link/gpio.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

//class ButtonInterruptHandler : public InterruptHandler {

//  public:

//    ButtonInterruptHandler(std::vector<int> pinIndices, IOExpander::Port port = IOExpander::PORTA)
//      : InterruptHandler(pinIndices, port)
//    {}

//    bool HandleInterrupt(uint8_t flag, uint8_t state, IOExpander &expander) override {
//        if ((flag & state) != 0) {
//          // sleep
//          std::this_thread::sleep_for(std::chrono::milliseconds(1));
//          // check again
//          if ((expander.ReadPort(m_port) | flag) != 0) {
//              std::cout << "Button on!" << std::endl;
//              return true;
//          }
//        }
//        return false;
//    }
//};

class UserInterface {

  public:

    enum class InputEvent {
      PlayStop,
      TapTempo,
      EncoderDown,
      EncoderUp,
      EncoderPress
    };

    UserInterface();
    virtual ~UserInterface();

    // Inputs

    void StartPollingInput();
    void StopPollingInput();

    // This will be called from input polling thread
    std::function<void(InputEvent)> onInputEvent;

    // Outputs

    void SetBPMModeLED(bool on);
    void SetLoopModeLED(bool on);
    void SetClockModeLED(bool on);

    // Index of animation LED starting from 0
    void SetAnimationLED(int index, bool on);

    void SetClock(bool on);
    void SetReset(bool on);

  private:

    std::shared_ptr<IOExpander> m_pExpander;
    std::unique_ptr<GPIO::Pin> m_pClockOut;
    std::unique_ptr<GPIO::Pin> m_pResetOut;
};

} // namespace
