#pragma once

#if FMI_VERSION != 2 && FMI_VERSION != 3
#error FMI_VERSION must be either 2 or 3
#endif

#define UNUSED(x) (void)(x)

#include <stddef.h>  // for size_t
#include <stdbool.h> // for bool
#include <stdint.h>

#include "config.h"

typedef enum {
    Instantiated           = 1 << 0,
    ConfigurationMode      = 1 << 1,
    InitializationMode     = 1 << 2,
    ReconfigurationMode    = 1 << 3,
    EventMode              = 1 << 4,
    Terminated             = 1 << 5,

    // Model Exchange
    ContinuousTimeMode     = 1 << 6,

    // Co-Simulation
    StepMode               = 1 << 7,
    IntermediateUpdateMode = 1 << 8,

    // Scheduled Execution
    ClockActivationMode    = 1 << 9,
    ClockUpdateMode        = 1 << 10,
} ModelState;

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

typedef void (*lockPreemptionType)   (void);
typedef void (*unlockPreemptionType) (void);


typedef void (*intermediateUpdateType) (void *instanceEnvironment,
                                        double intermediateUpdateTime,
                                        bool intermediateVariableSetRequested,
                                        bool intermediateVariableGetAllowed,
                                        bool intermediateStepFinished,
                                        bool canReturnEarly,
                                        bool *earlyReturnRequested,
                                        double *earlyReturnTime);

typedef void(*clockUpdateType) (void *instanceEnvironment);

typedef struct ModelInstance ModelInstance;

ModelInstance *createModelExchangeInstance(
    loggerType logger,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn);

ModelInstance *createCoSimulationInstance(
    loggerType logger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    bool earlyReturnAllowed,
    bool eventModeUsed);

ModelInstance *createScheduledExecutionInstance(
    loggerType logger,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    clockUpdateType clockUpdate,
    lockPreemptionType lockPreemption,
    unlockPreemptionType unlockPreemption);

ModelInstance *createModelInstance(
    loggerType logger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    InterfaceType interfaceType);

void freeModelInstance(ModelInstance *instance);

Status configurate(ModelInstance *instance);

Status reset(ModelInstance *instance);

Status setupExperiment(
    ModelInstance *instance,
    bool toleranceDefined,
    double tolerance,
    double startTime,
    bool stopTimeDefined,
    double stopTime
);

Status enterConfigurationMode(ModelInstance *instance);

Status exitConfigurationMode(ModelInstance *instance);

Status enterInitializationMode(ModelInstance *instance);

Status exitInitializationMode(ModelInstance *instance);

Status enterEventMode(ModelInstance *instance);

Status enterContinuousTimeMode(ModelInstance *instance);

Status terminate(ModelInstance *instance);

Status completedIntegratorStep(ModelInstance *instance,
    bool  noSetFMUStatePriorToCurrentPoint,
    bool* enterEventMode,
    bool* terminateSimulation);

Status setTime(ModelInstance *instance, double time);

Status enterStepMode(ModelInstance *instance);

Status setStartValues(ModelInstance *instance);

Status calculateValues(ModelInstance *instance);

Status getVariableSize(ModelInstance* instance, ValueReference vr, size_t* size);

Status getFloat32 (ModelInstance *instance, ValueReference vr, float       values[], size_t nValues, size_t *index);
Status getFloat64 (ModelInstance *instance, ValueReference vr, double      values[], size_t nValues, size_t *index);
Status getInt8    (ModelInstance *instance, ValueReference vr, int8_t      values[], size_t nValues, size_t *index);
Status getUInt8   (ModelInstance *instance, ValueReference vr, uint8_t     values[], size_t nValues, size_t *index);
Status getInt16   (ModelInstance *instance, ValueReference vr, int16_t     values[], size_t nValues, size_t *index);
Status getUInt16  (ModelInstance *instance, ValueReference vr, uint16_t    values[], size_t nValues, size_t *index);
Status getInt32   (ModelInstance *instance, ValueReference vr, int32_t     values[], size_t nValues, size_t *index);
Status getUInt32  (ModelInstance *instance, ValueReference vr, uint32_t    values[], size_t nValues, size_t *index);
Status getInt64   (ModelInstance *instance, ValueReference vr, int64_t     values[], size_t nValues, size_t *index);
Status getUInt64  (ModelInstance *instance, ValueReference vr, uint64_t    values[], size_t nValues, size_t *index);
Status getBoolean (ModelInstance *instance, ValueReference vr, bool        values[], size_t nValues, size_t *index);
Status getString  (ModelInstance *instance, ValueReference vr, const char* values[], size_t nValues, size_t *index);
Status getBinary  (ModelInstance *instance, ValueReference vr, size_t sizes[], const char* values[], size_t nValues, size_t *index);

Status setFloat32 (ModelInstance *instance, ValueReference vr, const float       values[], size_t nValues, size_t *index);
Status setFloat64 (ModelInstance *instance, ValueReference vr, const double      values[], size_t nValues, size_t *index);
Status setInt8    (ModelInstance *instance, ValueReference vr, const int8_t      values[], size_t nValues, size_t *index);
Status setUInt8   (ModelInstance *instance, ValueReference vr, const uint8_t     values[], size_t nValues, size_t *index);
Status setInt16   (ModelInstance *instance, ValueReference vr, const int16_t     values[], size_t nValues, size_t *index);
Status setUInt16  (ModelInstance *instance, ValueReference vr, const uint16_t    values[], size_t nValues, size_t *index);
Status setInt32   (ModelInstance *instance, ValueReference vr, const int32_t     values[], size_t nValues, size_t *index);
Status setUInt32  (ModelInstance *instance, ValueReference vr, const uint32_t    values[], size_t nValues, size_t *index);
Status setInt64   (ModelInstance *instance, ValueReference vr, const int64_t     values[], size_t nValues, size_t *index);
Status setUInt64  (ModelInstance *instance, ValueReference vr, const uint64_t    values[], size_t nValues, size_t *index);
Status setBoolean (ModelInstance *instance, ValueReference vr, const bool        values[], size_t nValues, size_t *index);
Status setString  (ModelInstance *instance, ValueReference vr, const char* const values[], size_t nValues, size_t *index);
Status setBinary  (ModelInstance *instance, ValueReference vr, const size_t sizes[], const char* const values[], size_t nValues, size_t *index);

Status activateClock(ModelInstance *instance, ValueReference vr);
Status getClock(ModelInstance *instance, ValueReference vr, bool* value);
Status setClock(ModelInstance *instance, ValueReference vr, const bool* value);

Status getInterval(ModelInstance *instance, ValueReference vr, double* interval, int* qualifier);

Status activateModelPartition(ModelInstance *instance, ValueReference vr, double activationTime);

Status getNumberOfEventIndicators(ModelInstance *instance, size_t* nEventIndicators);
Status getNumberOfContinuousStates(ModelInstance *instance, size_t* nContinuousStates);
Status getContinuousStates(ModelInstance *instance, double x[], size_t nx);
Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx);
Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx);
Status getDerivatives(ModelInstance *instance, double dx[], size_t nx);
Status getOutputDerivative(ModelInstance *instance, ValueReference valueReference, int order, double *value);
Status getPartialDerivative(ModelInstance *instance, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double *partialDerivative);
Status getEventIndicators(ModelInstance *instance, double z[], size_t nz);

Status updateDiscreteStates(
    ModelInstance *instance,
    bool* discreteStatesNeedUpdate,
    bool* terminateSimulation,
    bool* nominalsOfContinuousStatesChanged,
    bool* valuesOfContinuousStatesChanged,
    bool* nextEventTimeDefined,
    double* nextEventTime
);

bool isClose(double a, double b);
bool invalidNumber(ModelInstance *instance, const char *f, const char *arg, size_t actual, size_t expected);
bool invalidState(ModelInstance *instance, const char *f, int statesExpected);
bool nullPointer(ModelInstance *instance, const char *f, const char *arg, const void *p);
void logError(ModelInstance *instance, const char *message, ...);
Status setDebugLogging(ModelInstance *instance, bool loggingOn, size_t nCategories, const char * const categories[]);
void logEvent(ModelInstance *instance, const char *message, ...);
void logError(ModelInstance *instance, const char *message, ...);

Status freeFMUState(ModelInstance* instance, void* FMUState);
Status getFMUState(ModelInstance *instance, void** FMUState);
Status setFMUState(ModelInstance *instance, void* FMUState);
Status serializedFMUStateSize(ModelInstance* instance, void* FMUState, size_t* size);
Status serializeFMUState(
    ModelInstance *instance,
    void* FMUState,
    uint8_t serializedState[],
    size_t size);
Status deserializeFMUState(
    ModelInstance *instance,
    const uint8_t serializedState[],
    size_t size,
    void** FMUState);


// shorthand to access the variables
#define M(v) (instance->modelData.v)

// "stringification" macros
#define xstr(s) str(s)
#define str(s) #s

#define ASSERT_NOT_NULL2(V) \
    if (!V) { \
        logError(instance, "Argument '" xstr(V) "' must not be NULL"); \
        return Error; \
    }

// assert size of nValues for scalar variables
#define ASSERT_NVALUES(N) \
    do { \
        const size_t expected_nValues = *index + (N); \
        if (expected_nValues > nValues) { \
            logError(instance, "Expected nValues >= %zu but was %zu.", expected_nValues, nValues); \
            return Error; \
        } \
    } while (0)

#define ASSERT_SIZE_T(A, E) \
    if (A != E) { \
        logError(instance, "Expected " xstr(A) " = %zu but was %zu.", E, A); \
        return Error; \
    }
