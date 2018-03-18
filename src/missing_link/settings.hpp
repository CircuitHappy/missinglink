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
  int ppqn_index;
  int ppqn;
  static const std::vector<int> ppqn_options;


  // Defaults
  Settings() : tempo(120.0), quantum(4), ppqn_index(4) {}

  // Load from config file
  static Settings Load();

  // Save to config file
  static void Save(const Settings settings);
};

}
