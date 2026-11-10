#pragma once

#include "model.h"

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
#if N_EVENT_INDICATORS > 0
    size_t nz;
    double z[N_EVENT_INDICATORS];
    double prez[N_EVENT_INDICATORS];
#endif

#if N_CONTINUOUS_STATES > 0
    size_t nx;
    double x[N_CONTINUOUS_STATES];
    double dx[N_CONTINUOUS_STATES];
#endif

};

// shorthand to access the variables
#define M(v) (instance->modelData.v)

// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

#define ASSERT_NOT_NULL2(V) \
    if (!V) { \
        logError(instance, "Argument '" xstr(V) "' must not be NULL"); \
        return Discard; \
    }

// assert size of nValues for scalar variables
#define ASSERT_NVALUES(N) \
    do { \
        const size_t expected_nValues = *index + (N); \
        if (expected_nValues > nValues) { \
            logError(instance, "Expected nValues >= %zu but was %zu.", expected_nValues, nValues); \
            return Discard; \
        } \
    } while (0)

#define ASSERT_SIZE_T(A, E) \
    if (A != E) { \
        logError(instance, "Expected " xstr(A) " = %zu but was %zu.", E, A); \
        return Discard; \
    }
