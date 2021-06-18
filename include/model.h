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
#include "namespace.h"
#include "fmi3Functions.h"

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


// Internal functions that make up an FMI independent layer.
// These are prefixed to enable static linking.
#define createModelInstance   fmi3FullName(createModelInstance)
#define freeModelInstance   fmi3FullName(freeModelInstance)
#define invalidNumber   fmi3FullName(invalidNumber)
#define invalidState   fmi3FullName(invalidState)
#define nullPointer   fmi3FullName(nullPointer)
#define setDebugLogging   fmi3FullName(setDebugLogging)
#define logMessage   fmi3FullName(logMessage)
#define logEvent   fmi3FullName(logEvent)
#define logError   fmi3FullName(logError)
#define getEventIndicators   fmi3FullName(getEventIndicators)
#define getUInt16   fmi3FullName(getUInt16)
#define getFloat64   fmi3FullName(getFloat64)
#define getInt32   fmi3FullName(getInt32)
#define getBoolean   fmi3FullName(getBoolean)
#define getString   fmi3FullName(getString)
#define getBinary   fmi3FullName(getBinary)
#define setFloat64   fmi3FullName(setFloat64)
#define setUInt16   fmi3FullName(setUInt16)
#define setInt32   fmi3FullName(setInt32)
#define setBoolean   fmi3FullName(setBoolean)
#define setString   fmi3FullName(setString)
#define setBinary   fmi3FullName(setBinary)
#define activateClock   fmi3FullName(activateClock)
#define getClock   fmi3FullName(getClock)
#define getInterval   fmi3FullName(getInterval)
#define activateModelPartition   fmi3FullName(activateModelPartition)
#define getContinuousStates   fmi3FullName(getContinuousStates)
#define setContinuousStates   fmi3FullName(setContinuousStates)
#define getDerivatives   fmi3FullName(getDerivatives)
#define getPartialDerivative   fmi3FullName(getPartialDerivative)
#define doStep   fmi3FullName(doStep)

// Declarations of internal functions
ModelInstance* createModelInstance( loggerType cbLogger, intermediateUpdateType intermediateUpdate, void* componentEnvironment, const char* instanceName, const char* instantiationToken, const char* resourceLocation, bool loggingOn, InterfaceType interfaceType, bool returnEarly);
void freeModelInstance(ModelInstance *comp);
bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, size_t actual, size_t expected);
bool invalidState(ModelInstance *comp, const char *f, int statesExpected);
bool nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p);
Status setDebugLogging(ModelInstance *comp, bool loggingOn, size_t nCategories, const char * const categories[]);
static void logMessage(ModelInstance *comp, int status, const char *category, const char *message, va_list args);
void logEvent(ModelInstance *comp, const char *message, ...);
void logError(ModelInstance *comp, const char *message, ...);
void getEventIndicators(ModelInstance *comp, double z[], size_t nz);
Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index);
Status getUInt16(ModelInstance* comp, ValueReference vr, uint16_t *value, size_t *index);
Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index);
Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index);
Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index);
Status getBinary(ModelInstance* comp, ValueReference vr, size_t size[], const char *value[], size_t *index);
Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index);
Status setUInt16(ModelInstance* comp, ValueReference vr, const uint16_t *value, size_t *index);
Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index);
Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index);
Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index);
Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char *const value[], size_t *index);
Status activateClock(ModelInstance* comp, ValueReference vr);
Status getClock(ModelInstance* comp, ValueReference vr, bool* value);
Status getInterval(ModelInstance* comp, ValueReference vr, double* interval, int* qualifier);
Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime);
void getContinuousStates(ModelInstance *comp, double x[], size_t nx);
void setContinuousStates(ModelInstance *comp, const double x[], size_t nx);
void getDerivatives(ModelInstance *comp, double dx[], size_t nx);
Status getPartialDerivative(ModelInstance *comp, ValueReference unknown, ValueReference known, double *partialDerivative);
Status doStep(ModelInstance *comp, double t, double tNext, int* earlyReturn, double* lastSuccessfulTime);


// Functions to be implemented by the includer of this file:
#define setStartValues   fmi3FullName(setStartValues)
#define calculateValues   fmi3FullName(calculateValues)
#define eventUpdate   fmi3FullName(eventUpdate)

void setStartValues(ModelInstance *comp);
void calculateValues(ModelInstance *comp);
void eventUpdate(ModelInstance *comp);



#endif  /* model_h */
