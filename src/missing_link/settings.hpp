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
  static const std::vector<int> ppqn_options;
  int delay_compensation;


  // Defaults
  Settings() : tempo(120.0), quantum(4), ppqn_index(2), delay_compensation(0) {}

  // Load from config file
  static Settings Load();

  // Save to config file
  static void Save(const Settings settings);

  //look up ppqn value in ppqn_options vector
  int getPPQN() const;
};

}
