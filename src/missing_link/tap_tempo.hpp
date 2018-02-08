/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#include <chrono>
#include <functional>

namespace MissingLink {

class TapTempo {

public:

  TapTempo();
  virtual ~TapTempo();

  void Tap();

  std::function<void(double)> onNewTempo;

private:

  typedef std::chrono::time_point<std::chrono::steady_clock> timestamp;

  int m_tapCount;
  timestamp m_startTapTime;
  timestamp m_previousTapTime;

};

}
