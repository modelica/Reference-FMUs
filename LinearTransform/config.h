#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER LinearTransform
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f000}"

// define model size
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

#define GET_FLOAT64
#define SET_FLOAT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1

typedef enum {
    vr_m, vr_n, vr_u, vr_A, vr_y
} ValueReference;

typedef struct {

    int m;
    int n;
    double u[3];
    double A[2][3];
    double y[2];

} ModelData;

#endif /* config_h */
