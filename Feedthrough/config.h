#ifndef config_h
#define config_h

#include <stddef.h>  // for size_t
#include <stdbool.h> // for bool


// define class name and unique id
#define MODEL_IDENTIFIER Feedthrough
#define INSTANTIATION_TOKEN "{8c4e810f-3df3-4a00-8276-176fa3c9f004}"

#define CO_SIMULATION
#define MODEL_EXCHANGE

// define model size
#define NX 0
#define NZ 0

#define GET_FLOAT64
#define GET_INT32
#define GET_BOOLEAN
#define GET_STRING
#define GET_BINARY

#define SET_FLOAT64
#define SET_INT32
#define SET_BOOLEAN
#define SET_STRING
#define SET_BINARY

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
    vr_binary_in,
    vr_binary_out,
} ValueReference;

typedef struct {
    double      real_fixed_parameter;
    double      real_tunable_parameter;
    double      real_continuous_in;
    double      real_discrete;
    int         integer;
    bool        boolean;
    const char *string;
    size_t      binary_size;
    const char *binary;
} ModelData;

extern const char *STRING_START;
extern const char *BINARY_START;

#endif /* config_h */
