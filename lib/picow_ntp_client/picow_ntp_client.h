#ifndef PICOW_NTP_CLIENT_H
#define PICOW_NTP_CLIENT_H

#include "pico/stdlib.h"
#include "lwipopts.h"
#include "pico/cyw43_arch.h"

#ifdef __cplusplus
extern "C" {
#endif

void checkNTP(void);
int setupNTP(void);
int closeNTP(void);

#ifdef __cplusplus
}
#endif

#endif