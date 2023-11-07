#ifndef config_h
#define config_h

#include <stdint.h>

// define class name and unique id
#define MODEL_IDENTIFIER LinearTransform
#define INSTANTIATION_TOKEN "{D773325B-AB94-4630-BF85-643EB24FCB78}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define SET_FLOAT64
#define GET_UINT64
#define SET_UINT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1
#define DEFAULT_STOP_TIME 10

#define M_MAX 5
#define N_MAX 5

typedef enum {
    vr_time,
    vr_m,
    vr_n,
    vr_u,
    vr_A,
    vr_y
} ValueReference;

typedef struct {
    uint64_t m;
    uint64_t n;
    double u[N_MAX];
    double A[M_MAX][N_MAX];
    double y[M_MAX];
} ModelData;

#endif /* config_h */
