#ifndef config_h
#define config_h

#include "stdint.h"

// define class name and unique id
#define MODEL_IDENTIFIER Clocks
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f000}"

// define model size
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

#define EVENT_UPDATE
#define ACTIVATE_CLOCK
#define GET_CLOCK


#define FIXED_SOLVER_STEP 1

typedef enum {
	vr_c1,
	vr_c2,
	vr_c3,
	vr_c4,
	vr_c1Ticks,
	vr_c2Ticks,
	vr_totalTicks,
} ValueReference;

typedef struct {
    int c1;
    int c2;
    int c3;
    int c4;
	int c1Ticks;
	int c2Ticks;
	int totalTicks;
} ModelData;

#endif /* config_h */
