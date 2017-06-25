/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 *
 * I2C interface for NHD-0216AW-IB3 OLED character display.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void oled_init();
void oled_write(unsigned int line, const char *text);
void oled_clear();

#ifdef __cplusplus
}
#endif

