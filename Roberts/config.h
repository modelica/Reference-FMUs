#ifndef config_h
#define config_h

#include <stdbool.h>

// define class name and unique id
#define MODEL_IDENTIFIER Roberts
#define INSTANTIATION_TOKEN "{1AE5E10D-9521-4DE3-80B9-D0EAAA7D5AF2}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define MAX_CONTINUOUS_STATES 2
#define MAX_EVENT_INDICATORS 1

#define FIXED_SOLVER_STEP 1e-6
#define DEFAULT_STOP_TIME 1

#define GET_PARTIAL_DERIVATIVE

#define SET_FLOAT64
#define GET_BOOLEAN
#define SET_BOOLEAN

typedef enum {
    vr_time,
    vr_dae,
    vr_y1,
    vr_der_y1,
    vr_y2,
    vr_der_y2,
    vr_y3,
    vr_y3_nominal,
    vr_r,
    vr_g1,
    vr_g2,
} ValueReference;

typedef struct {
    bool dae;
    double y1;
    double der_y1;
    double y2;
    double der_y2;
    double y3;
    double r;
    double g1;
    double g2;
} ModelData;

#endif /* config_h */
