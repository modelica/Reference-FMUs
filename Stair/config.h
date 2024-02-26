#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Stair
#define INSTANTIATION_TOKEN "{BD403596-3166-4232-ABC2-132BDF73E644}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define GET_INT32
#define SET_INT32

#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 0.2
#define DEFAULT_STOP_TIME 10

typedef enum {
    vr_time, vr_counter
} ValueReference;

typedef struct {

    int counter;

} ModelData;

#endif /* config_h */
