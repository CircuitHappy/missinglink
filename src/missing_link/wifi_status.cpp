/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include "missing_link/wifi_status.hpp"

using namespace MissingLink;
using namespace MissingLink::FileIO;

WifiStatus::WifiStatus()
  : m_pWifiStatusFile(std::unique_ptr<TextFile>(new TextFile(wifiStatusFile)))
{
}

WifiStatus::~WifiStatus() {}

WifiState WifiStatus::ReadStatus() {
  std::string strState = m_pWifiStatusFile->Read();
  if (strState == "NO_WIFI_FOUND") {
    return NO_WIFI_FOUND;
  }
  if (strState == "WIFI_CONNECTED") {
      return WIFI_CONNECTED;
  }
  if (strState == "TRYING_TO_CONNECT") {
      return TRYING_TO_CONNECT;
  }
  if (strState == "AP_MODE") {
      return AP_MODE;
  }
  //DEFAULT STATE
  return NO_WIFI_FOUND;
}
