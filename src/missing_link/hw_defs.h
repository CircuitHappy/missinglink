#pragma once

// Pin numbers are unix GPIO pin num, e.g. /sys/class/gpio6

#define ML_DEFAULT_I2C_BUS  0
#define ML_INTERRUPT_PIN    14
#define ML_CLOCK_PIN        15
#define ML_RESET_PIN        16

// Length of reset pulse
#define ML_RESET_PULSE_LENGTH 0.05 // seconds
