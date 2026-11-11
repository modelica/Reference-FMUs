#pragma once

#include "model.h"

#ifdef N_EVENT_INDICATORS
#define MAX_EVENT_INDICATORS N_EVENT_INDICATORS
#endif

#ifdef N_CONTINUOUS_STATES
#define MAX_CONTINUOUS_STATES N_CONTINUOUS_STATES
#endif

typedef enum {
    Instantiated = 1 << 0,
    ConfigurationMode = 1 << 1,
    InitializationMode = 1 << 2,
    ReconfigurationMode = 1 << 3,
    EventMode = 1 << 4,
    Terminated = 1 << 5,

    // Model Exchange
    ContinuousTimeMode = 1 << 6,

    // Co-Simulation
    StepMode = 1 << 7,
    IntermediateUpdateMode = 1 << 8,

    // Scheduled Execution
    ClockActivationMode = 1 << 9,
    ClockUpdateMode = 1 << 10,
} ModelState;

struct ModelInstance {
    double startTime;
    bool stopTimeDefined;
    double stopTime;
    double time;
    const char *instanceName;
    InterfaceType type;
    const char *resourceLocation;

    // callback functions
    loggerType logger;
    intermediateUpdateType intermediateUpdate;
    clockUpdateType clockUpdate;

    lockPreemptionType lockPreemption;
    unlockPreemptionType unlockPreemption;

    bool logEvents;
    bool logErrors;

    void *componentEnvironment;
    ModelState state;

    bool clocksTicked;

    bool isDirtyValues;

    ModelData modelData;

    // internal solver steps
    uint64_t nSteps;

    // Co-Simulation
    bool earlyReturnAllowed;
    bool eventModeUsed;
    bool nextEventTimeDefined;
    double nextEventTime;
    double nextCommunicationPoint;

    // solver
#ifdef MAX_EVENT_INDICATORS
    size_t nz;
    double z[MAX_EVENT_INDICATORS];
    double prez[MAX_EVENT_INDICATORS];
#endif

#ifdef MAX_CONTINUOUS_STATES
    size_t nx;
    double x[MAX_CONTINUOUS_STATES];
    double dx[MAX_CONTINUOUS_STATES];
#endif
};

// shorthand to access the variables
#define M(v) (instance->modelData.v)

#define UNUSED(x) (void)(x)

// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

#define ASSERT_NOT_NULL(V)                                             \
    if (!V) {                                                          \
        logError(instance, "Argument '" xstr(V) "' must not be NULL"); \
        return Discard;                                                \
    }

#define ASSERT_SIZE_T(A, E)                                                  \
    if (A != E) {                                                            \
        logError(instance, "Expected " xstr(A) " = %zu but was %zu.", E, A); \
        return Discard;                                                      \
    }
