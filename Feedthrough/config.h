#ifndef config_h
#define config_h

#include <stdbool.h> // for bool


// define class name and unique id
#define MODEL_IDENTIFIER Feedthrough
#define MODEL_GUID "{8c4e810f-3df3-4a00-8276-176fa3c9f004}"

// define model size
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

#define GET_FLOAT64
#define GET_INT32
#define GET_BOOLEAN
#define GET_STRING

#define SET_FLOAT64
#define SET_INT32
#define SET_BOOLEAN
#define SET_STRING

#define EVENT_UPDATE

#define FIXED_SOLVER_STEP 1

typedef enum {
    vr_fixed_real_parameter,
    vr_tunable_real_parameter,
    vr_continuous_real_in,
    vr_continuous_real_out,
    vr_discrete_real_in,
    vr_discrete_real_out,
    vr_int_in,
    vr_int_out,
    vr_bool_in,
    vr_bool_out,
    vr_string,
} ValueReference;

typedef struct {
    double      real_fixed_parameter;
    double      real_tunable_parameter;
    double      real_continuous_in;
    double      real_discrete;
    int         integer;
    bool        boolean;
    const char *string;
} ModelData;

extern const char *STRING_START;

#endif /* config_h */
