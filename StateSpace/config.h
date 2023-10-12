#ifndef config_h
#define config_h

#include <stdint.h>

// define class name and unique id
#define MODEL_IDENTIFIER StateSpace
#define INSTANTIATION_TOKEN "{D773325B-AB94-4630-BF85-643EB24FCB78}"

#define HAS_CONTINUOUS_STATES

#define CO_SIMULATION
#define MODEL_EXCHANGE

#define SET_FLOAT64
#define GET_UINT64
#define SET_UINT64
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1e-3
#define DEFAULT_STOP_TIME 10

#define M_MAX 5
#define N_MAX 5
#define R_MAX 5

typedef enum {
    vr_time,
    vr_m,
    vr_n,
    vr_r,
    vr_A,
    vr_B,
    vr_C,
    vr_D,
    vr_x0,
    vr_u,
    vr_y,
    vr_x,
    vr_der_x
} ValueReference;

typedef struct {
    uint64_t m;
    uint64_t n;
    uint64_t r;
    double A[M_MAX][N_MAX];
    double B[M_MAX][N_MAX];
    double C[M_MAX][N_MAX];
    double D[M_MAX][N_MAX];
    double x0[N_MAX];
    double u[N_MAX];
    double y[N_MAX];
    double x[N_MAX];
    double der_x[N_MAX];
} ModelData;

#endif /* config_h */
