#ifndef config_h
#define config_h

#include "stdint.h"


// define class name and unique id
#define MODEL_IDENTIFIER Clocks
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f000}"

// define model size
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

#define N_INPUT_CLOCKS  3
typedef enum {
	InClock_1 = 0,
	InClock_2 = 1,
	InClock_3 = 2
}InputClockEnum;

#define N_OUTPUT_CLOCKS 2
typedef enum {
	OutClock_1 = 0,
	OutClock_2 = 1
}OutputClockEnum;

#define N_INPUTS  2
#define N_OUTPUTS 4


#define SET_INT32
#define GET_INT32
#define GET_CLOCK
#define ACTIVATEMODELPARTITION


#define FIXED_SOLVER_STEP 1

typedef enum {
	vr_c1 = 1001,
	vr_c2 = 1002,
	vr_c3 = 1003,
	vr_c4 = 1004,
	vr_c5 = 1005,
	vr_c1Ticks = 2001,
	vr_c2Ticks = 2002,
	vr_c3Ticks = 2003,
	vr_totalInTicks = 2004,
	vr_boost_c2 = 2012,
	vr_boost_c3 = 2013
} ValueReference;

#define NUMMODELPART N_INPUT_CLOCKS
#define MAXNUMVARPERPART  3

typedef struct {
    int c1;
    int c2;
    int c3;
	int c4;
	int c5;
	int c1Ticks;
	int c2Ticks; 
	int c3Ticks;
    int totalInTicks;
	int boost_c2;
	int boost_c3;
} ModelData;

typedef struct threadArgs_t {
	void * comp;
	ValueReference clockRef; 
	unsigned int *retval;
	double activationTime;
} ThreadArgs;

#endif /* config_h */
