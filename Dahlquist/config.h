#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Dahlquist
#define INSTANTIATION_TOKEN "{221063D2-EF4A-45FE-B954-B5BFEEA9A59B}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define HAS_CONTINUOUS_STATES

#define SET_FLOAT64

#define FIXED_SOLVER_STEP 0.1
#define DEFAULT_STOP_TIME 10

typedef enum {
    vr_time, vr_x, vr_der_x, vr_k
} ValueReference;

typedef struct {

    double x;
    double der_x;
    double k;

} ModelData;

#endif /* config_h */
