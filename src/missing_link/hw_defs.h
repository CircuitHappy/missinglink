#pragma once

// Pin numbers are unix GPIO pin num, e.g. /sys/class/gpio6

#define ML_DEFAULT_I2C_BUS  1
#define ML_INTERRUPT_PIN    25
#define ML_CLOCK_PIN        23
#define ML_RESET_PIN        24

// Length of reset pulse
#define ML_RESET_PULSE_LENGTH 0.05 // seconds
