// system_info.cpp
#include <string>
//#include <cstdlib>
#include <iostream>
#include "missing_link/system_info.hpp"

using namespace MissingLink;

SysInfo::SysInfo()
{
  open();
}

SysInfo::~SysInfo() {
  close();
}

std::string SysInfo::GetIP() {
  bool foundWlan = false;
  FILE * fp = popen("ifconfig uap0", "r");
  std::string result = "000.000.000.000";
  std::string line;
  if (fp) {
    char *p=NULL; size_t n;
    while ((getline(&p, &n, fp) > 0) && p) {
      line = (std::string)p;
      if (line.find("inet ") != std::string::npos) {
        int start = line.find("inet ") + 5;
        int stop = line.find(" netmask") - start;
        result = line.substr(line.find("inet ") + 5, stop);
        std::cout << "WLAN IP: " + result << std::endl;
        foundWlan = true;
        break;
      }
    }
    pclose(fp);
  }
  if (foundWlan == false) {
    fp = popen("ifconfig wlan0", "r");
    if (fp) {
      char *p=NULL; size_t n;
      while ((getline(&p, &n, fp) > 0) && p) {
        line = (std::string)p;
        if (line.find("inet ") != std::string::npos) {
          int start = line.find("inet ") + 5;
          int stop = line.find(" netmask") - start;
          result = line.substr(line.find("inet ") + 5, stop);
          std::cout << "WLAN IP: " + result << std::endl;
          foundWlan = true;
          break;
        }
      }
      pclose(fp);
    }
  }
  return result;
}

void SysInfo::open() {
  // do something?
}

void SysInfo::close() {
  //maybe do something?
}
