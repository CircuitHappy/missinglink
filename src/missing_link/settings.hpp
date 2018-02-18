/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

namespace MissingLink {

/// POD struct represeting persistent link engine settings
struct Settings {

  double tempo;
  int quantum;
  int ppqn;

  // Defaults
  Settings() : tempo(120.0), quantum(4), ppqn(4) {}

  // Load from config file
  static Settings Load();

  // Save to config file
  static void Save(const Settings settings);
};

}

