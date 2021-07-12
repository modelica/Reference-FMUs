#ifndef config_h
#define config_h

#define MODEL_IDENTIFIER ControlledClocksCS
#define INSTANTIATION_TOKEN "{9c4e810f-3df3-4a00-8276-176fa3c9f000}"

#define WINDOWS

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define NX 0
#define NZ 0

#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1e-3

#define GET_INT32
#define SET_CLOCK  //see co-simulation.c

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
    vr_output2 = 2004

} ValueReference;

typedef struct {

    _Bool startTask1;
    _Bool endTask1;
    _Bool startTask2;
    _Bool endTask2;
    _Bool startTask3;
    _Bool endTask3;
    _Bool startTask4;
    _Bool endTask4;
    _Bool startTask5;
    _Bool endTask5;

    int output1;
    int output2;	

} ModelData;


#endif /* config_h */
