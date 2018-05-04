/**
 * Copyright (c) 2018
 * Circuit Happy, LLC
 */

#pragma once

#include <chrono>
#include <mutex>

namespace MissingLink {
  typedef std::chrono::steady_clock Clock;
  typedef std::chrono::time_point<Clock> TimePoint;
  typedef std::lock_guard<std::mutex> ScopedMutex;
}
