#define _POSIX_C_SOURCE 199309L
#ifndef TIME_BACK_H
#define TIME_BACK_H

#include <time.h>
#include <stdlib.h>
#include "defs.h"

static uint64_t initTime = 0;       // local init time(ms)
static uint64_t randOffTime = 0;    // rand time(ms) for diff between vehicles

typedef union {
	uint8_t raw[5];
	uint64_t full;
	struct {
		uint32_t low32;
		uint8_t high8;
	} __attribute__((packed));
	struct {
		uint8_t low8;
		uint32_t high32;
	} __attribute__((packed));
} dwTime_t;

uint64_t get_current_milliseconds();
void precise_msleep(uint64_t milliseconds);
void iniTimeSetting();
uint64_t getCurrentTime();

#endif