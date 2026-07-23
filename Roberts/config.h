#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Roberts
#define INSTANTIATION_TOKEN "{1AE5E10D-9521-4DE3-80B9-D0EAAA7D5AF2}"

#define MODEL_EXCHANGE

#define MAX_CONTINUOUS_STATES 2
#define MAX_EVENT_INDICATORS 2

#define SET_FLOAT64

#define FIXED_SOLVER_STEP 1e-3
#define DEFAULT_STOP_TIME 3

typedef enum {
    vr_time,
    vr_y1,
    vr_der_y1,
    vr_y2,
    vr_der_y2,
    vr_y3,
    vr_r
} ValueReference;

typedef struct {
    double y1;
    double der_y1;
    double y2;
    double der_y2;
    double y3;
    double r;
} ModelData;

#endif /* config_h */
