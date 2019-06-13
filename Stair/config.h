#ifndef config_h
#define config_h

// define class name and unique id
#define MODEL_IDENTIFIER Stair
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f008}"

// define model size
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

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
