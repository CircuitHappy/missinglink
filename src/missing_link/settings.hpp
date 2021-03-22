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
  int reset_mode;
  static const std::vector<int> ppqn_options;
  int delay_compensation;
  bool start_stop_sync;
  int ap_mode;
  int outa_mode;
  int outb_mode;
  int outa_ppqn;
  int outb_ppqn;

  // Defaults
  Settings() 
  : tempo(120.0)
  , quantum(4)
  , ppqn_index(2)
  , reset_mode(0)
  , delay_compensation(0)
  , start_stop_sync(false)
  , ap_mode(0) 
  , outa_mode(0)
  , outb_mode(2)
  , outa_ppqn(4)
  , outb_ppqn(4)
  {}

  // Load from config file
  static Settings Load();

  // Save to config file
  static void Save(const Settings settings);

  //look up ppqn value in ppqn_options vector
  int getPPQN() const;

};

}
