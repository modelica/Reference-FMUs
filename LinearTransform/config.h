#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER LinearTransform
#define INSTANTIATION_TOKEN "{8c4e810f-3df3-4a00-8276-176fa3c9f000}"

// define model size
#define NX 0
#define NZ 0

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define GET_FLOAT64
#define SET_FLOAT64
#define GET_INT32
#define SET_INT32
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1

#define M_MAX 5
#define N_MAX 5

typedef enum {
    vr_m, vr_n, vr_u, vr_A, vr_y
} ValueReference;

typedef struct {

    int m;
    int n;
    double u[N_MAX];
    double A[M_MAX][N_MAX];
    double y[M_MAX];

} ModelData;

#endif /* config_h */
