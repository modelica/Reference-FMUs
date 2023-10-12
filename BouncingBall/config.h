#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER BouncingBall
#define INSTANTIATION_TOKEN "{1AE5E10D-9521-4DE3-80B9-D0EAAA7D5AF1}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define HAS_CONTINUOUS_STATES
#define HAS_EVENT_INDICATORS

#define SET_FLOAT64
#define GET_OUTPUT_DERIVATIVE
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1e-3
#define DEFAULT_STOP_TIME 3

typedef enum {
    vr_time, vr_h, vr_der_h, vr_v, vr_der_v, vr_g, vr_e, vr_v_min
} ValueReference;

typedef struct {

    double h;
    double v;
    double g;
    double e;

} ModelData;

#endif /* config_h */
