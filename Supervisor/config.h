#ifndef config_h
#define config_h

#include <stdbool.h> // for bool

// define class name and unique id
#define MODEL_IDENTIFIER Supervisor
#define INSTANTIATION_TOKEN "{64202d14-799a-4379-9fb3-79354aec17b2}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 0
#define NZ 1

#define SET_FLOAT64
#define GET_FLOAT64
#define EVENT_UPDATE
#define GET_CLOCK

#define FIXED_SOLVER_STEP 1e-2

typedef enum {
    vr_s = 1,   // Clock s
    vr_x,       // Sample from Plant
    vr_as       // Output that is fed to the Controller
} ValueReference;

typedef struct {
    bool s;    // Clock
    double x;  // Sample
    double as; // Output that is fed to the Controller
    bool clock_s_ticking; // Keeps track of whether clock s is ticking during event mode.
    double z; // Event indicator
    double pz; // Previous Event Indicator
} ModelData;

#endif /* config_h */
