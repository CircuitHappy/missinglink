#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int i2c_open(unsigned int bus);
int i2c_init(int fd, int addr);

#ifdef __cplusplus
}
#endif
