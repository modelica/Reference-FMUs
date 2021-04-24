#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Plant
#define INSTANTIATION_TOKEN "{00000000-0000-0000-0000-000000000000}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 1
#define NZ 0

#define GET_FLOAT64
#define SET_FLOAT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1e-3

typedef enum {
    vr_x = 1, vr_der_x, vr_u
} ValueReference;

typedef struct {
    double x;
    double der_x;
    double u;
} ModelData;

#endif /* config_h */
