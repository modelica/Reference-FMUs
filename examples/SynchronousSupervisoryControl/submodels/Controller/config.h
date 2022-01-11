#ifndef config_h
#define config_h

#include <stdbool.h> // for bool

// define class name and unique id
#define MODEL_IDENTIFIER Controller
#define INSTANTIATION_TOKEN "{00000000-0000-0000-0000-000000000000}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 0
#define NZ 0

#define GET_FLOAT64
#define SET_FLOAT64
#define EVENT_UPDATE
#define ACTIVATE_CLOCK
#define GET_CLOCK

#define FIXED_SOLVER_STEP 1e-3

typedef enum {
    vr_r = 1,   // Clock
    vr_xr,      // Sample
    vr_ur,      // Discrete state/output
    vr_pre_ur,  // Previous ur
    vr_ar       // Local var
} ValueReference;

typedef struct {
    bool r;    // Clock
    double xr;      // Sample
    double ur;      // Discrete state/output
    double pre_ur;  // Previous ur
    double ar;      // Local var
} ModelData;

#endif /* config_h */
