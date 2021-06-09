#ifndef config_h
#define config_h

#include "stdint.h"

// define class name and unique id
#define MODEL_IDENTIFIER ControlledClocksSE
#define INSTANTIATION_TOKEN "{8c4e810f-3df3-4a00-8276-176fa3c9f002}"

#define WINDOWS

#define SCHEDULED_CO_SIMULATION

// define model size
#define NX 0
#define NZ 0

#define EVENT_UPDATE
#define ACTIVATE_CLOCK
#define GET_INT32
#define GET_CLOCK
#define ACTIVATE_MODEL_PARTITION
#define N_INPUT_CLOCKS 10
#define N_OUTPUT_CLOCKS 0

#define FIXED_SOLVER_STEP 1

typedef enum {
    vr_startTask1 = 1001,
    vr_endTask1 = 1002,
    vr_startTask2 = 1003,
	vr_endTask2 = 1004,
    vr_startTask3 = 1005,
    vr_endTask3 = 1006,
	vr_startTask4 = 1007,
    vr_endTask4 = 1008,
    vr_startTask5 = 1009,
	vr_endTask5 = 1010,
	vr_output1 = 2003,
	vr_output2 = 2004,
} ValueReference;

typedef struct {
	int output1;
    int output2;
    
} ModelData;


#endif /* config_h */
