#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Dahlquist
#define INSTANTIATION_TOKEN "{8c4e810f-3df3-4a00-8276-176fa3c9f000}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 1
#define NZ 0

#define GET_FLOAT64
#define SET_FLOAT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 0.1

typedef enum {
    vr_x, vr_der_x, vr_k
} ValueReference;

typedef struct {

    double x;
    double der_x;
    double k;

} ModelData;

#endif /* config_h */
