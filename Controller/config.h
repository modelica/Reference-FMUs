#ifndef config_h
#define config_h

#include <stdbool.h> // for bool

// define class name and unique id
#define MODEL_IDENTIFIER Controller
#define INSTANTIATION_TOKEN "{e1f14bf0-302d-4ef9-b11c-e01c7ed456cb}"

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

#define FIXED_SOLVER_STEP 1e-2

typedef enum {
    vr_r = 1,   // Clock
    vr_xr,      // Sample
    vr_ur,      // Discrete state/output
    vr_pre_ur,  // Previous ur
    vr_as,      // Local var
    vr_s        // Clock from supervisor
} ValueReference;

typedef struct {
    bool r;         // Clock
    double xr;      // Sample
    double ur;      // Discrete state/output
    double pre_ur;  // Previous ur
    double as;      // Local var
    bool s;         // Clock from supervisor
} ModelData;

#endif /* config_h */
