#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Stair
#define INSTANTIATION_TOKEN "{8c4e810f-3df3-4a00-8276-176fa3c9f008}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 0
#define NZ 0

#define GET_INT32
#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 0.2

typedef enum {
    vr_counter
} ValueReference;

typedef struct {

    int counter;

} ModelData;

#endif /* config_h */
