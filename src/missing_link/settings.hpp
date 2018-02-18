/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <libconfig.h++>

namespace MissingLink {

/// POD struct represeting persistent link engine settings
struct Settings {

  double tempo;
  int quantum;
  int ppqn;

  // Defaults
  Settings() : tempo(120.0), quantum(4), ppqn(4) {}

  // Load from config file
  static Settings Load() {
    return Settings();
  }

  // Save to config file
  static void Save(Settings settings) {

  }

  private:

};

}

