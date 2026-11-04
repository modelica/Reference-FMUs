#pragma once

/* Internal model layer: full ModelInstance layout. Not part of the FMI API. */

#include "model.h"

struct ModelInstance {

    double startTime;
    bool stopTimeDefined;
    double stopTime;
    double time;
    const char *instanceName;
    InterfaceType type;
    const char *resourceLocation;

    Status status;

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
    double nextCommunicationPoint;

    // solver
#if MAX_EVENT_INDICATORS > 0
    size_t nz;
    double z[MAX_EVENT_INDICATORS];
    double prez[MAX_EVENT_INDICATORS];
#endif

#if MAX_CONTINUOUS_STATES > 0
    size_t nx;
    double x[MAX_CONTINUOUS_STATES];
    double dx[MAX_CONTINUOUS_STATES];
#endif

};
