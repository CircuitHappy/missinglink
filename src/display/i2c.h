#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int i2c_write(int bus, int addr, const unsigned char *data, size_t nbytes);

#ifdef __cplusplus
}
#endif
