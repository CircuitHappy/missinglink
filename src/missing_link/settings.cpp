/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <libconfig.h++>
#include "missing_link/settings.hpp"

#define ML_CONFIG_FILE "/etc/missing_link.cfg"

using namespace libconfig;
using namespace MissingLink;

const std::vector<int> Settings :: ppqn_options ({1, 2, 4, 8, 12, 16, 24, 32});

Settings Settings::Load() {
  Settings settings;
  Config config;
  bool valid = false;

  try {
    config.readFile(ML_CONFIG_FILE);
    valid = true;
  } catch (const FileIOException &exc) {
    std::cerr << "Failed to read config file" << std::endl;
  } catch (const ParseException &exc) {
    std::cerr << "Failed to parse config file" << std::endl;
  }

  if (!valid) {
    // indiscriminately delete file and return defaults
    ::remove(ML_CONFIG_FILE);
    return settings;
  }

  try {
    settings.tempo = config.lookup("tempo");
    settings.quantum = config.lookup("quantum");
    settings.ppqn_index = config.lookup("ppqn_index");
    settings.reset_mode = config.lookup("reset_mode");
  } catch (const SettingNotFoundException &exc) {
    std::cerr << "One or more settings missing from config file" << std::endl;
  }

  std::cout << std::setprecision(1) << std::setiosflags(std::ios::fixed) <<
    "Loaded Settings:" <<
    "\n  tempo: " << settings.tempo <<
    "\n  quantum: " << settings.quantum <<
    "\n  ppqn: " << settings.getPPQN() <<
    "\n  reset_mode: " << settings.reset_mode << std::endl;

  return settings;
}

void Settings::Save(const Settings settings) {
  FILE *file = fopen(ML_CONFIG_FILE, "wt");
  if (file == NULL) {
    std::cerr << "Failed to open config file for writing" << std::endl;
    return;
  }

  Config config;

  Setting &root = config.getRoot();
  root.add("tempo", Setting::TypeFloat) = settings.tempo;
  root.add("quantum", Setting::TypeInt) = settings.quantum;
  root.add("ppqn_index", Setting::TypeInt) = settings.ppqn_index;
  root.add("reset_mode", Setting::TypeInt) = settings.reset_mode;

  try {
    config.write(file);
  } catch (const FileIOException &exc) {
    std::cerr << "Failed to write settings to config file" << std::endl;
  }

  // Explicitly synchronize file
  fflush(file);
  int fd = fileno(file);
  if (fd >= 0) {
    fsync(fd);
  }

  fclose(file);
}

int Settings::getPPQN() const {
  return ppqn_options[ppqn_index];
}
