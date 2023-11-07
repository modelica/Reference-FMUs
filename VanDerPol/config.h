#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER VanDerPol
#define INSTANTIATION_TOKEN "{BD403596-3166-4232-ABC2-132BDF73E644}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define HAS_CONTINUOUS_STATES

#define SET_FLOAT64

#define GET_PARTIAL_DERIVATIVE

#define FIXED_SOLVER_STEP 1e-2
#define DEFAULT_STOP_TIME 20

typedef enum {
    vr_time, vr_x0, vr_der_x0, vr_x1, vr_der_x1, vr_mu
} ValueReference;

typedef struct {

    double x0;
    double der_x0;
    double x1;
    double der_x1;
    double mu;

} ModelData;

#endif /* config_h */
