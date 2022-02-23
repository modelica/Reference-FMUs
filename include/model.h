#ifndef model_h
#define model_h

#if FMI_VERSION != 1 && FMI_VERSION != 2 && FMI_VERSION != 3
#error FMI_VERSION must be one of 1, 2 or 3
#endif

#define UNUSED(x) (void)(x)

#include <stddef.h>  // for size_t
#include <stdbool.h> // for bool
#include <stdint.h>

#include "config.h"
#include "namespace.h"

/* Macros to construct the real function name (prepend function name by FMI3_FUNCTION_PREFIX) */
#if defined(FMI3_FUNCTION_PREFIX)
#define fmi3Paste(a,b)     a ## b
#define fmi3PasteB(a,b)    fmi3Paste(a,b)
#define fmi3FullName(name) fmi3PasteB(FMI3_FUNCTION_PREFIX, name)
#else
#define fmi3FullName(name) name
#endif

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
typedef void (*loggerType) (void *componentEnvironment, int status, const char *category, const char *message);
#endif

typedef void (*lockPreemptionType)   ();
typedef void (*unlockPreemptionType) ();

typedef void (*intermediateUpdateType) (void *instanceEnvironment,
                                        double intermediateUpdateTime,
                                        bool intermediateVariableSetRequested,
                                        bool intermediateVariableGetAllowed,
                                        bool intermediateStepFinished,
                                        bool canReturnEarly,
                                        bool *earlyReturnRequested,
                                        double *earlyReturnTime);

typedef void(*clockUpdateType) (void *instanceEnvironment);

typedef struct {

    double time;
    const char *instanceName;
    InterfaceType type;
    const char *resourceLocation;

    Status status;

    // callback functions
    loggerType logger;
    intermediateUpdateType intermediateUpdate;
    clockUpdateType clockUpdate;

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

    // causes of entering event mode
    bool isBecauseOfTimeEvent;
    bool isBecauseOfStateEvent;


    bool isDirtyValues;

    ModelData *modelData;

    // event indicators
    double *z;
    double *prez;

    // internal solver steps
    int nSteps;

    // Co-Simulation
    bool earlyReturnAllowed;
    bool eventModeUsed;

} ModelInstance;

// Internal functions that make up an FMI independent layer.
// These are prefixed to enable static linking.
#define createModelInstance   fmi3FullName(createModelInstance)
#define freeModelInstance   fmi3FullName(freeModelInstance)
#define setStartValues   fmi3FullName(setStartValues)
#define calculateValues   fmi3FullName(calculateValues)
#define getFloat64   fmi3FullName(getFloat64)
#define getUInt16   fmi3FullName(getUInt16)
#define getInt32   fmi3FullName(getInt32)
#define getUInt64   fmi3FullName(getUInt64)
#define getBoolean   fmi3FullName(getBoolean)
#define getString   fmi3FullName(getString)
#define getBinary   fmi3FullName(getBinary)
#define setFloat64   fmi3FullName(setFloat64)
#define setUInt16   fmi3FullName(setUInt16)
#define setInt32   fmi3FullName(setInt32)
#define setUInt64   fmi3FullName(setUInt64)
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
#define getOutputDerivative   fmi3FullName(getOutputDerivative)
#define getPartialDerivative   fmi3FullName(getPartialDerivative)
#define getEventIndicators   fmi3FullName(getEventIndicators)
#define eventUpdate   fmi3FullName(eventUpdate)
#define epsilon   fmi3FullName(epsilon)
#define invalidNumber   fmi3FullName(invalidNumber)
#define invalidState   fmi3FullName(invalidState)
#define nullPointer   fmi3FullName(nullPointer)
#define logError   fmi3FullName(logError)
#define setDebugLogging   fmi3FullName(setDebugLogging)
#define logEvent   fmi3FullName(logEvent)

ModelInstance *createModelInstance(
    loggerType logger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    InterfaceType interfaceType);

void freeModelInstance(ModelInstance *comp);

void reset(ModelInstance* comp);

void setStartValues(ModelInstance* comp);

Status calculateValues(ModelInstance *comp);

Status getFloat64 (ModelInstance* comp, ValueReference vr, double      *value, size_t *index);
Status getUInt16  (ModelInstance* comp, ValueReference vr, uint16_t    *value, size_t *index);
Status getInt32   (ModelInstance* comp, ValueReference vr, int32_t     *value, size_t *index);
Status getUInt64  (ModelInstance* comp, ValueReference vr, uint64_t    *value, size_t *index);
Status getBoolean (ModelInstance* comp, ValueReference vr, bool        *value, size_t *index);
Status getString  (ModelInstance* comp, ValueReference vr, const char **value, size_t *index);
Status getBinary  (ModelInstance* comp, ValueReference vr, size_t size[], const char* value[], size_t *index);

Status setFloat64 (ModelInstance* comp, ValueReference vr, const double      *value, size_t *index);
Status setUInt16  (ModelInstance* comp, ValueReference vr, const uint16_t    *value, size_t *index);
Status setInt32   (ModelInstance* comp, ValueReference vr, const int32_t     *value, size_t *index);
Status setUInt64  (ModelInstance* comp, ValueReference vr, const uint64_t    *value, size_t *index);
Status setBoolean (ModelInstance* comp, ValueReference vr, const bool        *value, size_t *index);
Status setString  (ModelInstance* comp, ValueReference vr, const char* const *value, size_t *index);
Status setBinary  (ModelInstance* comp, ValueReference vr, const size_t size[], const char *const value[], size_t *index);

Status activateClock(ModelInstance* comp, ValueReference vr);
Status getClock(ModelInstance* comp, ValueReference vr, bool* value);

Status getInterval(ModelInstance* comp, ValueReference vr, double* interval, int* qualifier);

Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime);

void getContinuousStates(ModelInstance *comp, double x[], size_t nx);
void setContinuousStates(ModelInstance *comp, const double x[], size_t nx);
void getDerivatives(ModelInstance *comp, double dx[], size_t nx);
Status getOutputDerivative(ModelInstance *comp, ValueReference valueReference, int order, double *value);
Status getPartialDerivative(ModelInstance *comp, ValueReference unknown, ValueReference known, double *partialDerivative);
void getEventIndicators(ModelInstance *comp, double z[], size_t nz);
void eventUpdate(ModelInstance *comp);
//void updateEventTime(ModelInstance *comp);

double epsilon(double value);
bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, size_t actual, size_t expected);
bool invalidState(ModelInstance *comp, const char *f, int statesExpected);
bool nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p);
void logError(ModelInstance *comp, const char *message, ...);
Status setDebugLogging(ModelInstance *comp, bool loggingOn, size_t nCategories, const char * const categories[]);
void logEvent(ModelInstance *comp, const char *message, ...);
void logError(ModelInstance *comp, const char *message, ...);

// shorthand to access the variables
#define M(v) (comp->modelData->v)

// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

#endif  /* model_h */
