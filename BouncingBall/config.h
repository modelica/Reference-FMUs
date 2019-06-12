#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER BouncingBall
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f003}"

// define model size
#define NUMBER_OF_STATES 2
#define NUMBER_OF_EVENT_INDICATORS 1

#define GET_FLOAT64
#define SET_FLOAT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1e-3

typedef enum {
    vr_h, vr_v, vr_g, vr_e, vr_v_min
} ValueReference;

typedef struct {

    double h;
    double v;
    double g;
    double e;

} ModelData;

#endif /* config_h */
