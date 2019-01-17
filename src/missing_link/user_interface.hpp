/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <functional>
#include <vector>
#include "missing_link/gpio.hpp"
#include "missing_link/control.hpp"
#include "missing_link/io_expander.hpp"

namespace MissingLink {

class UserInputProcess : public Engine::Process {

  public:

    UserInputProcess(Engine &engine);

    // Outputs
    // These will be called from input polling thread
    std::function<void()> onPlayStop;
    std::function<void()> onTapTempo;
    std::function<void()> onEncoderAndTap;
    std::function<void()> onEncoderAndPlay;
    std::function<void()> onEncoderPress;
    std::function<void(float)> onEncoderRotate;

  private:

    void process() override;
    void handleInterrupt();

    std::vector<std::unique_ptr<Control>> m_controls;
    std::shared_ptr<IOExpander> m_pExpander;
    std::unique_ptr<GPIO::Pin> m_pInterruptIn;

    bool m_playButtonDown;
    bool m_tapButtonDown;
    bool m_encoderButtonDown;

    int m_loopGuardCount = 0;
};

} // namespace
