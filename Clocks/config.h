#ifndef config_h
#define config_h

#include "stdint.h"

// define class name and unique id
#define MODEL_IDENTIFIER Clocks
#define INSTANTIATION_TOKEN "{C5F142BA-B849-42DA-B4A1-4745BFF3BE28}"

#define WINDOWS

#define SCHEDULED_CO_SIMULATION

#define DEFAULT_STOP_TIME 3

#define EVENT_UPDATE
#define ACTIVATE_CLOCK
#define GET_INT32
#define SET_INT32
#define GET_CLOCK
#define GET_INTERVAL
#define ACTIVATE_MODEL_PARTITION
#define N_INPUT_CLOCKS 3

#define FIXED_SOLVER_STEP 1

typedef enum {
    vr_time              = 0,
    vr_inClock1          = 1001,
    vr_inClock2          = 1002,
    vr_inClock3          = 1003,
    vr_outClock1         = 1004,
    vr_outClock          = 1005,
    vr_inClock1Ticks     = 2001,
    vr_inClock2Ticks     = 2002,
    vr_inClock3Ticks     = 2003,
    vr_totalInClockTicks = 2004,
    vr_result2           = 2005,
    vr_input2            = 2006,
    vr_output3           = 2007,
} ValueReference;

typedef struct {
    double inClock3_interval;
    int inClock3_qualifier;
    int outClock;
    int inClock1Ticks;
    int inClock2Ticks;
    int inClock3Ticks;
    int totalInClockTicks;
    int result2;
    int input2;
    int output3;
} ModelData;


#endif /* config_h */
