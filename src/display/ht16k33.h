/**
 * Copyright (c) 2017
 * Circuit Happy, LLC
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// initialize display module
void ht16k33_init();

// write raw bitmask to display buffer digit at index
void ht16k33_write_raw(uint8_t index, uint16_t bitmask);

// write font-mapped ascii character to display buffer digit at index, optionally with dot
void ht16k33_write_ascii(uint8_t index, uint8_t a, bool dot);

// write full string to display buffer
// decimal points will automatically be converted appropriately
// strings shorter than 4 digits will be right-aligned
// strings longer than 4 digits will be truncated
void ht16k33_write_string(const char *string);

// commit (write) display buffer to display
void ht16k33_commit();

// clear display buffer and commit to display
void ht16k33_clear();

#ifdef __cplusplus
}
#endif

