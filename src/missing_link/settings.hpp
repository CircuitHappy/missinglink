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
  int reset_mode_index;
  static const std::vector<int> ppqn_options;
  static const std::vector<int> reset_mode_options;


  // Defaults
  Settings() : tempo(120.0), quantum(4), ppqn_index(2), reset_mode_index(0) {}

  // Load from config file
  static Settings Load();

  // Save to config file
  static void Save(const Settings settings);

  //look up ppqn value in ppqn_options vector
  int getPPQN() const;

  //look up reset_mode value in reset_mode_options vector
  int getResetMode() const;
};

}
