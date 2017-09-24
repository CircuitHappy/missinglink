/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#include <iostream>
#include "missing_link/gpio.hpp"
#include "missing_link/link_engine.hpp"

int main(void) {
  MissingLink::LinkEngine engine;
  engine.Run();
  return 0;
}
