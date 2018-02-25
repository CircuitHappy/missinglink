/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <thread>
#include <functional>
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

  private:

    std::shared_ptr<IOExpander> m_pExpander;
    std::unique_ptr<ExpanderInputLoop> m_pInputLoop;
};

} // namespace
