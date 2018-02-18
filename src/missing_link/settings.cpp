/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <unistd.h>
#include <libconfig.h++>
#include "missing_link/settings.hpp"

#define ML_CONFIG_FILE "/etc/missing_link.cfg"

using namespace libconfig;
using namespace MissingLink;

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
    settings.ppqn = config.lookup("ppqn");
  } catch (const SettingNotFoundException &exc) {
    std::cerr << "One or more settings missing from config file" << std::endl;
  }

  std::cout << std::setprecision(1) << std::setiosflags(std::ios::fixed) <<
    "Loaded Settings:" <<
    "\n  tempo: " << settings.tempo <<
    "\n  quantum: " << settings.quantum <<
    "\n  ppqn: " << settings.ppqn << std::endl;

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
  root.add("ppqn", Setting::TypeInt) = settings.ppqn;

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
