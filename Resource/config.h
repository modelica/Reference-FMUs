#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Resource
#define MODEL_GUID "{7b9c2114-2ce5-4076-a138-2cbc69e069e5}"

#define BASIC_CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 0
#define NZ 0

#define GET_FLOAT64

#define FIXED_SOLVER_STEP 1

typedef enum {
    vr_y
} ValueReference;

typedef struct {
    double y;
} ModelData;

#endif /* config_h */
