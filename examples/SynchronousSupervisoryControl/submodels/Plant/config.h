#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Plant
#define INSTANTIATION_TOKEN "{6e81b08d-97be-4de1-957f-8358a4e83184}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 1
#define NZ 0

#define GET_FLOAT64
#define SET_FLOAT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1e-2

typedef enum {
    vr_x = 1,
    vr_der_x,
    vr_u
} ValueReference;

typedef struct {
    double x;
    double der_x;
    double u;
} ModelData;

#endif /* config_h */
