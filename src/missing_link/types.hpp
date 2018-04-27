/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

namespace MissingLink {

    enum PlayState {
      Stopped,
      Cued,
      Playing,
      NUM_PLAY_STATES
    };

    enum InputMode {
      BPM,
      Loop,
      Clock,
      NUM_INPUT_MODES
    };

}
