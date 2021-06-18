#ifndef model_h
#define model_h

#if FMI_VERSION != 1 && FMI_VERSION != 2 && FMI_VERSION != 3
#error FMI_VERSION must be one of 1, 2 or 3
#endif

#define UNUSED(x) (void)(x);

#include <stddef.h>  // for size_t
#include <stdbool.h> // for bool
#include <stdint.h>

#include "config.h"

#if FMI_VERSION == 1

#define not_modelError (Instantiated| Initialized | Terminated)

typedef enum {
    Instantiated = 1<<0,
    Initialized  = 1<<1,
    Terminated   = 1<<2,
    modelError   = 1<<3
} ModelState;

#elif FMI_VERSION == 2

typedef enum {
    StartAndEnd        = 1<<0,
    Instantiated       = 1<<1,
    InitializationMode = 1<<2,

    // ME states
    EventMode          = 1<<3,
    ContinuousTimeMode = 1<<4,

    // CS states
    StepComplete       = 1<<5,
    StepInProgress     = 1<<6,
    StepFailed         = 1<<7,
    StepCanceled       = 1<<8,

    Terminated         = 1<<9,
    modelError         = 1<<10,
    modelFatal         = 1<<11,
} ModelState;

#else

typedef enum {
    StartAndEnd            = 1 << 0,
    ConfigurationMode      = 1 << 1,
    Instantiated           = 1 << 2,
    InitializationMode     = 1 << 3,
    EventMode              = 1 << 4,
    ContinuousTimeMode     = 1 << 5,
    StepMode               = 1 << 6,
    ClockActivationMode    = 1 << 7,
    StepDiscarded          = 1 << 8,
    ReconfigurationMode    = 1 << 9,
    IntermediateUpdateMode = 1 << 10,
    Terminated             = 1 << 11,
    modelError             = 1 << 12,
    modelFatal             = 1 << 13,
} ModelState;

#endif

typedef enum {
    ModelExchange,
    CoSimulation,
    ScheduledExecution,
} InterfaceType;

typedef enum {
    OK,
    Warning,
    Discard,
    Error,
    Fatal,
    Pending
} Status;

#if FMI_VERSION < 3
typedef void (*loggerType) (void *componentEnvironment, const char *instanceName, int status, const char *category, const char *message, ...);
#else
typedef void (*loggerType) (void *componentEnvironment, const char *instanceName, int status, const char *category, const char *message);
#endif

typedef void (*lockPreemptionType)   ();
typedef void (*unlockPreemptionType) ();

typedef void (*intermediateUpdateType) (void *instanceEnvironment,
                                        double intermediateUpdateTime,
                                        bool clocksTicked,
                                        bool intermediateVariableSetRequested,
                                        bool intermediateVariableGetAllowed,
                                        bool intermediateStepFinished,
                                        bool canReturnEarly,
                                        bool *earlyReturnRequested,
                                        double *earlyReturnTime);

typedef struct {

    double time;
    const char *instanceName;
    InterfaceType type;
    const char *resourceLocation;

    Status status;

    // callback functions
    loggerType logger;
    intermediateUpdateType intermediateUpdate;

    lockPreemptionType lockPreemtion;
    unlockPreemptionType unlockPreemtion;

    bool logEvents;
    bool logErrors;

    void *componentEnvironment;
    ModelState state;

    // event info
    bool newDiscreteStatesNeeded;
    bool terminateSimulation;
    bool nominalsOfContinuousStatesChanged;
    bool valuesOfContinuousStatesChanged;
    bool nextEventTimeDefined;
    double nextEventTime;
    bool clocksTicked;

    bool isDirtyValues;
    bool isNewEventIteration;

    ModelData *modelData;

    // event indicators
    double *z;
    double *prez;

    // internal solver steps
    int nSteps;

    // co-simulation
    bool returnEarly;

} ModelInstance;

// shorthand to access the variables
#define M(v) (comp->modelData->v)

#define GET_VARIABLES(T) \
size_t index = 0; \
Status status = OK; \
for (int i = 0; i < nvr; i++) { \
    if (vr[i] == 0) continue; \
    Status s = get ## T((ModelInstance *)instance, vr[i], value, &index); \
    status = max(status, s); \
    if (status > Warning) return (FMI_STATUS)status; \
} \
return (FMI_STATUS)status;

#define SET_VARIABLES(T) \
size_t index = 0; \
Status status = OK; \
for (int i = 0; i < nvr; i++) { \
    Status s = set ## T((ModelInstance *)instance, vr[i], value, &index); \
    status = max(status, s); \
    if (status > Warning) return (FMI_STATUS)status; \
} \
if (nvr > 0) ((ModelInstance *)instance)->isDirtyValues = true; \
return (FMI_STATUS)status;

// TODO: make this work with arrays
#define GET_BOOLEAN_VARIABLES \
Status status = OK; \
for (int i = 0; i < nvr; i++) { \
    bool v = false; \
    size_t index = 0; \
    Status s = getBoolean((ModelInstance *)instance, vr[i], &v, &index); \
    value[i] = v; \
    status = max(status, s); \
    if (status > Warning) return (FMI_STATUS)status; \
} \
return (FMI_STATUS)status;

// TODO: make this work with arrays
#define SET_BOOLEAN_VARIABLES \
Status status = OK; \
for (int i = 0; i < nvr; i++) { \
    bool v = value[i]; \
    size_t index = 0; \
    Status s = setBoolean((ModelInstance *)instance, vr[i], &v, &index); \
    status = max(status, s); \
    if (status > Warning) return (FMI_STATUS)status; \
} \
return (FMI_STATUS)status;

#endif  /* model_h */
