/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <memory>
#include <string>
#include <iostream>
#include "missing_link/file_io.hpp"

namespace MissingLink {

enum WifiState {
  NO_WIFI_FOUND       = 0,
  WIFI_CONNECTED      = 1,
  TRYING_TO_CONNECT   = 2,
  AP_MODE             = 3,
  REBOOT              = 4,
};

  class WifiStatus {

    public:

      WifiStatus();
      virtual ~WifiStatus();

      WifiState ReadStatus();

    private:
      std::string wifiStatusFile = "/tmp/WifiStatus";
      std::unique_ptr<FileIO::TextFile> m_pWifiStatusFile;
  };
}
