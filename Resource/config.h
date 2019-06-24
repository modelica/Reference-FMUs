#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Resource
#define MODEL_GUID "{7b9c2114-2ce5-4076-a138-2cbc69e069e5}"

// define model size
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

#define GET_FLOAT64

#define FIXED_SOLVER_STEP 1

typedef enum {
    vr_y
} ValueReference;

typedef struct {
    double y;
} ModelData;

#endif /* config_h */
