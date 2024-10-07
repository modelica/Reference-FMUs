#if FMI_VERSION != 3
#error FMI_VERSION must be 3
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#include "config.h"
#include "model.h"
#include "cosimulation.h"

// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#if !defined(DISABLE_PREFIX) && !defined(FMI3_FUNCTION_PREFIX)
#define pasteA(a,b)          a ## b
#define pasteB(a,b)          pasteA(a,b)
#define FMI3_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi3Functions.h"

#define ASSERT_NOT_NULL(p) \
do { \
    if (!p) { \
        logError(S, "Argument %s must not be NULL.", xstr(p)); \
        S->state = Terminated; \
        return (fmi3Status)Error; \
    } \
} while (0)

#define CALL(f) do { \
    const Status s = f; \
    if (s > status) { \
        status = s; \
    } \
    if (status == Discard) { \
        goto TERMINATE; \
    } else if (status == Error) { \
        S->state = Terminated; \
        goto TERMINATE; \
    } else if (status == Fatal) { \
        S->state = StartAndEnd; \
        goto TERMINATE; \
    } \
} while (false)

#define GET_VARIABLES(T) \
BEGIN_FUNCTION(Get ## T); \
if (nValueReferences == 0) goto TERMINATE; \
ASSERT_NOT_NULL(valueReferences); \
if (nValues > 0) ASSERT_NOT_NULL(values); \
if (S->isDirtyValues) { \
    CALL(calculateValues(S)); \
    S->isDirtyValues = false; \
} \
size_t index = 0; \
for (size_t i = 0; i < nValueReferences; i++) { \
    CALL(get ## T(S, (ValueReference)valueReferences[i], values, nValues, &index)); \
} \
if (index != nValues) { \
    logError(S, "Expected nValues = %zu but was %zu.", index, nValues); \
    CALL(Error); \
} \
END_FUNCTION()

#define SET_VARIABLES(T) \
BEGIN_FUNCTION(Set ## T); \
if (nValueReferences == 0) goto TERMINATE; \
ASSERT_NOT_NULL(valueReferences); \
size_t index = 0; \
for (size_t i = 0; i < nValueReferences; i++) { \
    CALL(set ## T(S, (ValueReference)valueReferences[i], values, nValues, &index)); \
} \
if (nValueReferences > 0) S->isDirtyValues = true; \
if (index != nValues) { \
    logError(S, "Expected nValues = %zu but was %zu.", index, nValues); \
    CALL(Error); \
} \
END_FUNCTION()

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Function calls allowed state masks for both Model-exchange and Co-simulation
// ---------------------------------------------------------------------------
#define MASK_AnyState                     (~0)

/* Inquire version numbers and set debug logging */
#define MASK_fmi3GetVersion               MASK_AnyState
#define MASK_fmi3SetDebugLogging          MASK_AnyState

/* Creation and destruction of FMU instances */
#define MASK_fmi3InstantiateInstantiateModelExchange MASK_AnyState
#define MASK_fmi3InstantiateCoSimulation             MASK_AnyState
#define MASK_fmi3InstantiateScheduledExectuion       MASK_AnyState
#define MASK_fmi3FreeInstance                        MASK_AnyState

/* Enter and exit initialization mode, terminate and reset */
#define MASK_fmi3EnterInitializationMode  Instantiated
#define MASK_fmi3ExitInitializationMode   InitializationMode
#define MASK_fmi3EnterEventMode           (ContinuousTimeMode | StepMode)
#define MASK_fmi3Terminate                (ContinuousTimeMode | StepMode | StepDiscarded | EventMode | ClockActivationMode | ReconfigurationMode)
#define MASK_fmi3Reset                    MASK_AnyState

/* Common Functions */

/* Getting and setting variable values */
#define MASK_fmi3GetFloat32               (Instantiated | InitializationMode | ConfigurationMode | ReconfigurationMode | EventMode | ContinuousTimeMode | StepMode | ClockActivationMode | IntermediateUpdateMode | Terminated)
#define MASK_fmi3GetFloat64               MASK_fmi3GetFloat32
#define MASK_fmi3GetInt8                  MASK_fmi3GetFloat32
#define MASK_fmi3GetUInt8                 MASK_fmi3GetFloat32
#define MASK_fmi3GetInt16                 MASK_fmi3GetFloat32
#define MASK_fmi3GetUInt16                MASK_fmi3GetFloat32
#define MASK_fmi3GetInt32                 MASK_fmi3GetFloat32
#define MASK_fmi3GetUInt32                MASK_fmi3GetFloat32
#define MASK_fmi3GetInt64                 MASK_fmi3GetFloat32
#define MASK_fmi3GetUInt64                MASK_fmi3GetFloat32
#define MASK_fmi3GetBoolean               MASK_fmi3GetFloat32
#define MASK_fmi3GetString                MASK_fmi3GetFloat32
#define MASK_fmi3GetBinary                MASK_fmi3GetFloat32
#define MASK_fmi3GetClock                 MASK_AnyState

#define MASK_fmi3SetFloat32               (Instantiated | InitializationMode | ConfigurationMode | ReconfigurationMode | EventMode | ContinuousTimeMode | StepMode | ClockActivationMode | IntermediateUpdateMode | Terminated)
#define MASK_fmi3SetFloat64               MASK_fmi3SetFloat32
#define MASK_fmi3SetInt8                  (Instantiated | ConfigurationMode | ReconfigurationMode | InitializationMode | EventMode | StepMode | ClockActivationMode | Terminated)
#define MASK_fmi3SetUInt8                 MASK_fmi3SetInt8
#define MASK_fmi3SetInt16                 MASK_fmi3SetInt8
#define MASK_fmi3SetUInt16                MASK_fmi3SetInt8
#define MASK_fmi3SetInt32                 MASK_fmi3SetInt8
#define MASK_fmi3SetUInt32                MASK_fmi3SetInt8
#define MASK_fmi3SetInt64                 MASK_fmi3SetInt8
#define MASK_fmi3SetUInt64                MASK_fmi3SetInt8
#define MASK_fmi3SetBoolean               MASK_fmi3SetInt8
#define MASK_fmi3SetString                MASK_fmi3SetInt8
#define MASK_fmi3SetBinary                MASK_fmi3SetInt8
#define MASK_fmi3SetClock                 MASK_AnyState

/* Getting Variable Dependency Information */
#define MASK_fmi3GetNumberOfVariableDependencies  MASK_AnyState
#define MASK_fmi3GetVariableDependencies          MASK_AnyState

/* Getting and setting the internal FMU state */
#define MASK_fmi3GetFMUState              MASK_AnyState
#define MASK_fmi3SetFMUState              MASK_AnyState
#define MASK_fmi3FreeFMUState             MASK_AnyState
#define MASK_fmi3SerializedFMUStateSize   MASK_AnyState
#define MASK_fmi3SerializeFMUState        MASK_AnyState
#define MASK_fmi3DeserializeFMUState      MASK_AnyState

/* Getting partial derivatives */
#define MASK_fmi3GetDirectionalDerivative (InitializationMode | StepMode | EventMode | ContinuousTimeMode | Terminated)
#define MASK_fmi3GetAdjointDerivative     MASK_fmi3GetDirectionalDerivative

/* Entering and exiting the Configuration or Reconfiguration Mode */
#define MASK_fmi3EnterConfigurationMode   (Instantiated | StepMode | EventMode | ClockActivationMode)
#define MASK_fmi3ExitConfigurationMode    (ConfigurationMode | ReconfigurationMode)

/* Clock related functions */
// TODO: fix masks
#define MASK_fmi3GetIntervalDecimal        MASK_AnyState
#define MASK_fmi3GetIntervalFraction       MASK_AnyState
#define MASK_fmi3SetIntervalDecimal        MASK_AnyState
#define MASK_fmi3SetIntervalFraction       MASK_AnyState
#define MASK_fmi3NewDiscreteStates         MASK_AnyState

/* Functions for Model Exchange */

#define MASK_fmi3EnterContinuousTimeMode       EventMode
#define MASK_fmi3CompletedIntegratorStep       ContinuousTimeMode

/* Providing independent variables and re-initialization of caching */
#define MASK_fmi3SetTime                       (EventMode | ContinuousTimeMode)
#define MASK_fmi3SetContinuousStates           ContinuousTimeMode

/* Evaluation of the model equations */
#define MASK_fmi3GetContinuousStateDerivatives (InitializationMode | EventMode | ContinuousTimeMode | Terminated)
#define MASK_fmi3GetEventIndicators            MASK_fmi3GetContinuousStateDerivatives
#define MASK_fmi3GetContinuousStates           MASK_fmi3GetContinuousStateDerivatives
#define MASK_fmi3GetNominalsOfContinuousStates MASK_fmi3GetContinuousStateDerivatives

#define MASK_fmi3GetNumberOfContinuousStates   MASK_AnyState
#define MASK_fmi3GetNumberOfEventIndicators    MASK_AnyState

/* Functions for Co-Simulation */

#define MASK_fmi3EnterStepMode            (InitializationMode | EventMode)
#define MASK_fmi3SetInputDerivatives      (Instantiated | InitializationMode | StepMode)
#define MASK_fmi3GetOutputDerivatives     (StepMode | StepDiscarded | Terminated)
#define MASK_fmi3DoStep                   StepMode
#define MASK_fmi3ActivateModelPartition   ClockActivationMode
#define MASK_fmi3DoEarlyReturn            IntermediateUpdateMode
#define MASK_fmi3GetDoStepDiscardedStatus StepMode

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

#define NOT_IMPLEMENTED \
do { \
    ModelInstance *comp = (ModelInstance *)instance; \
    logError(comp, "Function is not implemented."); \
    return fmi3Error; \
} while (0)

#define BEGIN_FUNCTION(F) \
Status status = OK; \
if (!instance) return fmi3Error; \
ModelInstance *S = (ModelInstance *)instance; \
if (!allowedState(S, MASK_fmi3##F, #F)) CALL(Error);

#define END_FUNCTION() \
TERMINATE: \
    return (fmi3Status)status;

static bool allowedState(ModelInstance *instance, int statesExpected, char *name) {

    if (!instance) {
        return false;
    }

    if (!(instance->state & statesExpected)) {
        logError(instance, "fmi3%s: Illegal call sequence.", name);
        return false;
    }

    return true;
}

/***************************************************
 Common Functions
 ****************************************************/

const char* fmi3GetVersion(void) {
    return fmi3Version;
}

fmi3Status fmi3SetDebugLogging(fmi3Instance instance,
                               fmi3Boolean loggingOn,
                               size_t nCategories,
                               const fmi3String categories[]) {

    BEGIN_FUNCTION(SetDebugLogging);

    CALL(setDebugLogging(S, loggingOn, nCategories, categories));

    END_FUNCTION();
}

fmi3Instance fmi3InstantiateModelExchange(
    fmi3String                 instanceName,
    fmi3String                 instantiationToken,
    fmi3String                 resourcePath,
    fmi3Boolean                visible,
    fmi3Boolean                loggingOn,
    fmi3InstanceEnvironment    instanceEnvironment,
    fmi3LogMessageCallback     logMessage) {

    UNUSED(visible);

#ifndef MODEL_EXCHANGE
    UNUSED(instanceName);
    UNUSED(instantiationToken);
    UNUSED(resourcePath);
    UNUSED(loggingOn);
    UNUSED(instanceEnvironment);
    UNUSED(logMessage);

    return NULL;
#else
    return createModelInstance(
        (loggerType)logMessage,
        NULL,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn,
        ModelExchange);
#endif
}

fmi3Instance fmi3InstantiateCoSimulation(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourcePath,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3Boolean                    eventModeUsed,
    fmi3Boolean                    earlyReturnAllowed,
    const fmi3ValueReference       requiredIntermediateVariables[],
    size_t                         nRequiredIntermediateVariables,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3LogMessageCallback         logMessage,
    fmi3IntermediateUpdateCallback intermediateUpdate) {

    UNUSED(visible);
    UNUSED(requiredIntermediateVariables);
    UNUSED(nRequiredIntermediateVariables);

#ifndef EVENT_UPDATE
    if (eventModeUsed) {
        if (logMessage) {
            logMessage(instanceEnvironment, fmi3Error, "error", "Event Mode is not supported.");
        }
        return NULL;
    }
#endif

    ModelInstance *instance = createModelInstance(
        (loggerType)logMessage,
        (intermediateUpdateType)intermediateUpdate,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn,
        CoSimulation);

    if (instance) {
        instance->earlyReturnAllowed = earlyReturnAllowed;
        instance->eventModeUsed      = eventModeUsed;
        instance->state              = Instantiated;
    }

    return instance;
}

fmi3Instance fmi3InstantiateScheduledExecution(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourcePath,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3LogMessageCallback         logMessage,
    fmi3ClockUpdateCallback        clockUpdate,
    fmi3LockPreemptionCallback     lockPreemption,
    fmi3UnlockPreemptionCallback   unlockPreemption) {

    UNUSED(visible);

#ifndef SCHEDULED_CO_SIMULATION

    UNUSED(instanceName);
    UNUSED(instantiationToken);
    UNUSED(resourcePath);
    UNUSED(loggingOn);
    UNUSED(instanceEnvironment);
    UNUSED(logMessage);
    UNUSED(clockUpdate);
    UNUSED(lockPreemption);
    UNUSED(unlockPreemption);

    return NULL;
#else
    ModelInstance *instance = createModelInstance(
        (loggerType)logMessage,
        NULL,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn,
        ScheduledExecution
    );

    if (instance) {
        instance->state = Instantiated;
        instance->clockUpdate = clockUpdate;
        instance->lockPreemption = lockPreemption;
        instance->unlockPreemption = unlockPreemption;
    }

    return instance;
#endif
}

void fmi3FreeInstance(fmi3Instance instance) {
    freeModelInstance((ModelInstance*)instance);
}

fmi3Status fmi3EnterInitializationMode(fmi3Instance instance,
                                       fmi3Boolean toleranceDefined,
                                       fmi3Float64 tolerance,
                                       fmi3Float64 startTime,
                                       fmi3Boolean stopTimeDefined,
                                       fmi3Float64 stopTime) {

    UNUSED(toleranceDefined);
    UNUSED(tolerance);

    BEGIN_FUNCTION(EnterInitializationMode);

    S->startTime = startTime;
    S->stopTime = stopTimeDefined ? stopTime : INFINITY;
    S->time = startTime;
    S->nextCommunicationPoint = startTime;
    S->state = InitializationMode;

    END_FUNCTION();
}

fmi3Status fmi3ExitInitializationMode(fmi3Instance instance) {

    BEGIN_FUNCTION(ExitInitializationMode);

    // if values were set and no fmi3GetXXX triggered update before,
    // ensure calculated values are updated now
    if (S->isDirtyValues) {
        CALL(calculateValues(S));
        S->isDirtyValues = false;
    }

    switch (S->type) {
        case ModelExchange:
            S->state = EventMode;
            break;
        case CoSimulation:
            S->state = S->eventModeUsed ? EventMode : StepMode;
            break;
        case ScheduledExecution:
            S->state = ClockActivationMode;
            break;
    }

    CALL(configurate(S));

    END_FUNCTION();
}

fmi3Status fmi3EnterEventMode(fmi3Instance instance) {

    BEGIN_FUNCTION(EnterEventMode);

    S->state = EventMode;

    END_FUNCTION();
}

fmi3Status fmi3Terminate(fmi3Instance instance) {

    BEGIN_FUNCTION(Terminate);

    S->state = Terminated;

    END_FUNCTION();
}

fmi3Status fmi3Reset(fmi3Instance instance) {

    BEGIN_FUNCTION(Reset);

    CALL(reset(S));

    END_FUNCTION();
}

fmi3Status fmi3GetFloat32(fmi3Instance instance,
                          const fmi3ValueReference valueReferences[],
                          size_t nValueReferences,
                          fmi3Float32 values[],
                          size_t nValues) {
    GET_VARIABLES(Float32);
}

fmi3Status fmi3GetFloat64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 values[],
    size_t nValues) {
    GET_VARIABLES(Float64);
}

fmi3Status fmi3GetInt8(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int8 values[],
    size_t nValues) {
    GET_VARIABLES(Int8);
}

fmi3Status fmi3GetUInt8(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt8 values[],
    size_t nValues) {
    GET_VARIABLES(UInt8);
}

fmi3Status fmi3GetInt16(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int16 values[],
    size_t nValues) {
    GET_VARIABLES(Int16);
}

fmi3Status fmi3GetUInt16(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt16 values[],
    size_t nValues) {
    GET_VARIABLES(UInt16);
}

fmi3Status fmi3GetInt32(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int32 values[],
    size_t nValues) {
    GET_VARIABLES(Int32);
}

fmi3Status fmi3GetUInt32(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt32 values[],
    size_t nValues) {
    GET_VARIABLES(UInt32);
}

fmi3Status fmi3GetInt64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int64 values[],
    size_t nValues) {
    GET_VARIABLES(Int64);
}

fmi3Status fmi3GetUInt64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt64 values[],
    size_t nValues) {
    GET_VARIABLES(UInt64);
}

fmi3Status fmi3GetBoolean(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Boolean values[],
    size_t nValues) {
    GET_VARIABLES(Boolean);
}

fmi3Status fmi3GetString(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3String values[],
    size_t nValues) {
    GET_VARIABLES(String);
}

fmi3Status fmi3GetBinary(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    size_t valueSizes[],
    fmi3Binary values[],
    size_t nValues) {

    BEGIN_FUNCTION(GetBinary);

    if (nValueReferences == 0) {
        goto TERMINATE;
    } else {
        ASSERT_NOT_NULL(valueReferences);
    }

    if (nValues > 0) {
        ASSERT_NOT_NULL(values);
    }

    if (S->isDirtyValues) {
        CALL(calculateValues(S));
        S->isDirtyValues = false;
    }

    size_t index = 0;

    for (size_t i = 0; i < nValueReferences; i++) {
        CALL(getBinary(S, (ValueReference)valueReferences[i], valueSizes, (const char**)values, nValues, &index));
    }

    END_FUNCTION();
}

fmi3Status fmi3GetClock(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Clock values[]) {

    BEGIN_FUNCTION(GetClock);

    for (size_t i = 0; i < nValueReferences; i++) {
        CALL(getClock(instance, (ValueReference)valueReferences[i], &values[i]));
    }
    END_FUNCTION();
}

fmi3Status fmi3SetFloat32(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float32 values[],
    size_t nValues) {
    SET_VARIABLES(Float32);
}

fmi3Status fmi3SetFloat64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float64 values[],
    size_t nValues) {
    SET_VARIABLES(Float64);
}

fmi3Status fmi3SetInt8(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int8 values[],
    size_t nValues) {
    SET_VARIABLES(Int8);
}

fmi3Status fmi3SetUInt8(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt8 values[],
    size_t nValues) {
    SET_VARIABLES(UInt8);
}

fmi3Status fmi3SetInt16(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int16 values[],
    size_t nValues) {
    SET_VARIABLES(Int16);
}

fmi3Status fmi3SetUInt16(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt16 values[],
    size_t nValues) {
    SET_VARIABLES(UInt16);
}

fmi3Status fmi3SetInt32(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int32 values[],
    size_t nValues) {
    SET_VARIABLES(Int32);
}

fmi3Status fmi3SetUInt32(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt32 values[],
    size_t nValues) {
    SET_VARIABLES(UInt32);
}

fmi3Status fmi3SetInt64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int64 values[],
    size_t nValues) {
    SET_VARIABLES(Int64);
}

fmi3Status fmi3SetUInt64(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt64 values[],
    size_t nValues) {
    SET_VARIABLES(UInt64);
}

fmi3Status fmi3SetBoolean(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Boolean values[],
    size_t nValues) {
    SET_VARIABLES(Boolean);
}

fmi3Status fmi3SetString(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3String values[],
    size_t nValues) {
    SET_VARIABLES(String);
}

fmi3Status fmi3SetBinary(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const size_t valueSizes[],
    const fmi3Binary values[],
    size_t nValues) {

    BEGIN_FUNCTION(SetBinary);

    if (nValueReferences == 0) {
        goto TERMINATE;
    }

    ASSERT_NOT_NULL(valueReferences);

    size_t index = 0;

    for (size_t i = 0; i < nValueReferences; i++) {
        CALL(setBinary(S, (ValueReference)valueReferences[i], valueSizes, (const char* const*)values, nValues, &index));
    }

    if (nValueReferences > 0) {
        S->isDirtyValues = true;
    }

    if (index != nValues) {
        logError(S, "Expected nValues = %zu but was %zu.", index, nValues); \
        CALL(Error);
    }

    END_FUNCTION();
}

fmi3Status fmi3SetClock(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Clock values[]) {

    BEGIN_FUNCTION(SetClock);

    for (size_t i = 0; i < nValueReferences; i++) {
        if (values[i]) {
            CALL(activateClock(instance, (ValueReference)valueReferences[i]));
        }
    }

    END_FUNCTION();
}

fmi3Status fmi3GetNumberOfVariableDependencies(fmi3Instance instance,
                                               fmi3ValueReference valueReference,
                                               size_t* nDependencies) {
    UNUSED(valueReference);
    UNUSED(nDependencies);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3GetVariableDependencies(fmi3Instance instance,
    fmi3ValueReference dependent,
    size_t elementIndicesOfDependent[],
    fmi3ValueReference independents[],
    size_t elementIndicesOfIndependents[],
    fmi3DependencyKind dependencyKinds[],
    size_t nDependencies) {

    UNUSED(dependent);
    UNUSED(elementIndicesOfDependent);
    UNUSED(independents);
    UNUSED(elementIndicesOfIndependents);
    UNUSED(dependencyKinds);
    UNUSED(nDependencies);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3GetFMUState(fmi3Instance instance, fmi3FMUState* FMUState) {

    BEGIN_FUNCTION(GetFMUState);

    CALL(getFMUState(S, FMUState));

    END_FUNCTION();
}

fmi3Status fmi3SetFMUState(fmi3Instance instance, fmi3FMUState FMUState) {

    BEGIN_FUNCTION(SetFMUState);

    if (nullPointer(S, "fmi3SetFMUState", "FMUState", FMUState)) {
        return fmi3Error;
    }

    CALL(setFMUState(S, FMUState));

    END_FUNCTION();
}

fmi3Status fmi3FreeFMUState(fmi3Instance instance, fmi3FMUState* FMUState) {

    BEGIN_FUNCTION(FreeFMUState);

    free(*FMUState);

    *FMUState = NULL;

    END_FUNCTION();
}

fmi3Status fmi3SerializedFMUStateSize(fmi3Instance instance,
    fmi3FMUState  FMUState,
    size_t* size) {

    UNUSED(instance);
    UNUSED(FMUState);

    BEGIN_FUNCTION(SerializedFMUStateSize);

    *size = sizeof(ModelInstance);

    END_FUNCTION();
}

fmi3Status fmi3SerializeFMUState(fmi3Instance instance,
    fmi3FMUState  FMUState,
    fmi3Byte serializedState[],
    size_t size) {

    BEGIN_FUNCTION(SerializeFMUState);

    if (nullPointer(S, "fmi3SerializeFMUState", "FMUstate", FMUState)) {
        return fmi3Error;
    }

    if (invalidNumber(S, "fmi3SerializeFMUState", "size", size, sizeof(ModelInstance))) {
        return fmi3Error;
    }

    memcpy(serializedState, FMUState, sizeof(ModelInstance));

    END_FUNCTION();
}

fmi3Status fmi3DeserializeFMUState(fmi3Instance instance,
    const fmi3Byte serializedState[],
    size_t size,
    fmi3FMUState* FMUState) {

    BEGIN_FUNCTION(DeserializeFMUState);

    if (invalidNumber(S, "fmi3DeserializeFMUState", "size", size, sizeof(ModelInstance))) {
        return fmi3Error;
    }

    if (*FMUState == NULL) {
        *FMUState = calloc(1, sizeof(ModelInstance));
        if (*FMUState == NULL) {
            printf("Failed to allocate memory for FMUState.\n");
            return fmi3Error;
        }
    }

    memcpy(*FMUState, serializedState, sizeof(ModelInstance));

    END_FUNCTION();
}

fmi3Status fmi3GetDirectionalDerivative(fmi3Instance instance,
    const fmi3ValueReference unknowns[],
    size_t nUnknowns,
    const fmi3ValueReference knowns[],
    size_t nKnowns,
    const fmi3Float64 seed[],
    size_t nSeed,
    fmi3Float64 sensitivity[],
    size_t nSensitivity) {

    UNUSED(nSeed);
    UNUSED(nSensitivity);

    BEGIN_FUNCTION(GetDirectionalDerivative);

    // TODO: check value references
    // TODO: assert nUnknowns == nDeltaOfUnknowns
    // TODO: assert nKnowns == nDeltaKnowns

    for (size_t i = 0; i < nUnknowns; i++) {
        sensitivity[i] = 0;
        for (size_t j = 0; j < nKnowns; j++) {
            double partialDerivative = 0;
            CALL(getPartialDerivative(S, (ValueReference)unknowns[i], (ValueReference)knowns[j], &partialDerivative));
            sensitivity[i] += partialDerivative * seed[j];
        }
    }

    END_FUNCTION();
}

fmi3Status fmi3GetAdjointDerivative(fmi3Instance instance,
    const fmi3ValueReference unknowns[],
    size_t nUnknowns,
    const fmi3ValueReference knowns[],
    size_t nKnowns,
    const fmi3Float64 seed[],
    size_t nSeed,
    fmi3Float64 sensitivity[],
    size_t nSensitivity) {

    UNUSED(nSeed);
    UNUSED(nSensitivity);

    BEGIN_FUNCTION(GetAdjointDerivative);

    // TODO: check value references

    for (size_t i = 0; i < nKnowns; i++) {
        sensitivity[i] = 0;
        for (size_t j = 0; j < nUnknowns; j++) {
            double partialDerivative = 0;
            CALL(getPartialDerivative(S, (ValueReference)unknowns[j], (ValueReference)knowns[i], &partialDerivative));
            sensitivity[i] += partialDerivative * seed[j];
        }
    }

    END_FUNCTION();
}

fmi3Status fmi3EnterConfigurationMode(fmi3Instance instance) {

    BEGIN_FUNCTION(EnterConfigurationMode);

    S->state = (S->state == Instantiated) ? ConfigurationMode : ReconfigurationMode;

    END_FUNCTION();
}

fmi3Status fmi3ExitConfigurationMode(fmi3Instance instance) {

    BEGIN_FUNCTION(ExitConfigurationMode);

    if (S->state == ConfigurationMode) {
        S->state = Instantiated;
    } else {
        switch (S->type) {
            case ModelExchange:
                S->state = EventMode;
                break;
            case CoSimulation:
                S->state = StepMode;
                break;
            case ScheduledExecution:
                S->state = ClockActivationMode;
                break;
        }
    }

#if defined(HAS_CONTINUOUS_STATES) || defined(HAS_EVENT_INDICATORS)
    CALL(configurate(S));
#endif

    END_FUNCTION();
}

fmi3Status fmi3GetIntervalDecimal(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 intervals[],
    fmi3IntervalQualifier qualifiers[]) {

    BEGIN_FUNCTION(GetIntervalDecimal);

    // TODO: Check nValueReferences != nValues ?

    for (size_t i = 0; i < nValueReferences; i++) {
        CALL(getInterval(instance, (ValueReference)valueReferences[i], &intervals[i], (int*)&qualifiers[i]));
    }

    END_FUNCTION();
}

fmi3Status fmi3GetIntervalFraction(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt64 intervalCounters[],
    fmi3UInt64 resolutions[],
    fmi3IntervalQualifier qualifiers[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(intervalCounters);
    UNUSED(resolutions);
    UNUSED(qualifiers);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3GetShiftDecimal(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 shifts[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(shifts);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3GetShiftFraction(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt64 shiftCounters[],
    fmi3UInt64 resolutions[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(shiftCounters);
    UNUSED(resolutions);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3SetIntervalDecimal(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float64 intervals[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(intervals);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3SetIntervalFraction(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt64 intervalCounters[],
    const fmi3UInt64 resolutions[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(intervalCounters);
    UNUSED(resolutions);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3SetShiftDecimal(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float64 shifts[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(shifts);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3SetShiftFraction(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt64 shiftCounters[],
    const fmi3UInt64 resolutions[]) {

    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(shiftCounters);
    UNUSED(resolutions);

    NOT_IMPLEMENTED;
}

fmi3Status fmi3EvaluateDiscreteStates(fmi3Instance instance) {
    NOT_IMPLEMENTED;
}

fmi3Status fmi3UpdateDiscreteStates(fmi3Instance instance,
    fmi3Boolean* discreteStatesNeedUpdate,
    fmi3Boolean* terminateSimulation,
    fmi3Boolean* nominalsOfContinuousStatesChanged,
    fmi3Boolean* valuesOfContinuousStatesChanged,
    fmi3Boolean* nextEventTimeDefined,
    fmi3Float64* nextEventTime) {

    BEGIN_FUNCTION(NewDiscreteStates);

#ifdef EVENT_UPDATE
    CALL(eventUpdate(S));
#endif

    // copy internal eventInfo of component to output arguments
    if (discreteStatesNeedUpdate)          *discreteStatesNeedUpdate          = S->newDiscreteStatesNeeded;
    if (terminateSimulation)               *terminateSimulation               = S->terminateSimulation;
    if (nominalsOfContinuousStatesChanged) *nominalsOfContinuousStatesChanged = S->nominalsOfContinuousStatesChanged;
    if (valuesOfContinuousStatesChanged)   *valuesOfContinuousStatesChanged   = S->valuesOfContinuousStatesChanged;
    if (nextEventTimeDefined)              *nextEventTimeDefined              = S->nextEventTimeDefined;
    if (nextEventTime)                     *nextEventTime                     = S->nextEventTime;

    END_FUNCTION();
}

/***************************************************
 Functions for Model Exchange
 ****************************************************/

fmi3Status fmi3EnterContinuousTimeMode(fmi3Instance instance) {

    BEGIN_FUNCTION(EnterContinuousTimeMode);

    S->state = ContinuousTimeMode;

    END_FUNCTION();
}

fmi3Status fmi3CompletedIntegratorStep(fmi3Instance instance,
    fmi3Boolean  noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* enterEventMode,
    fmi3Boolean* terminateSimulation) {

    UNUSED(noSetFMUStatePriorToCurrentPoint);

    BEGIN_FUNCTION(CompletedIntegratorStep);

    ASSERT_NOT_NULL(enterEventMode);
    ASSERT_NOT_NULL(terminateSimulation);

    *enterEventMode = fmi3False;
    *terminateSimulation = fmi3False;

    END_FUNCTION();
}

/* Providing independent variables and re-initialization of caching */
fmi3Status fmi3SetTime(fmi3Instance instance, fmi3Float64 time) {

    BEGIN_FUNCTION(SetTime);

    S->time = time;

    END_FUNCTION();
}

fmi3Status fmi3SetContinuousStates(fmi3Instance instance,
    const fmi3Float64 continuousStates[],
    size_t nContinuousStates) {

    BEGIN_FUNCTION(SetContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi3SetContinuousStates", "nContinuousStates", nContinuousStates, getNumberOfContinuousStates(S)))
        return fmi3Error;

    ASSERT_NOT_NULL(continuousStates);

    CALL(setContinuousStates(S, continuousStates, nContinuousStates));
#else
    UNUSED(continuousStates);
    UNUSED(nContinuousStates);
    return fmi3Error;
#endif

    END_FUNCTION();
}

/* Evaluation of the model equations */
fmi3Status fmi3GetContinuousStateDerivatives(fmi3Instance instance,
    fmi3Float64 derivatives[],
    size_t nContinuousStates) {

    BEGIN_FUNCTION(GetContinuousStateDerivatives);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi3GetContinuousStateDerivatives", "nContinuousStates", nContinuousStates, getNumberOfContinuousStates(S)))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetContinuousStateDerivatives", "derivatives[]", derivatives))
        return fmi3Error;

    CALL(getDerivatives(S, derivatives, nContinuousStates));
#else
    UNUSED(derivatives);
    UNUSED(nContinuousStates);
    return fmi3Error;
#endif

    END_FUNCTION();
}

fmi3Status fmi3GetEventIndicators(fmi3Instance instance,
    fmi3Float64 eventIndicators[],
    size_t nEventIndicators) {

    BEGIN_FUNCTION(GetEventIndicators);

#ifdef HAS_EVENT_INDICATORS
    if (invalidNumber(S, "fmi3GetEventIndicators", "nEventIndicators", nEventIndicators, getNumberOfEventIndicators(S))) {
        return fmi3Error;
    }

    CALL(getEventIndicators(S, eventIndicators, nEventIndicators));
#else

    UNUSED(eventIndicators);

    if (nEventIndicators > 0) {
        // TODO: log error
        return fmi3Error;
    }
#endif

    END_FUNCTION();
}

fmi3Status fmi3GetContinuousStates(fmi3Instance instance,
    fmi3Float64 continuousStates[],
    size_t nContinuousStates) {

    BEGIN_FUNCTION(GetContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi3GetContinuousStates", "nContinuousStates", nContinuousStates, getNumberOfContinuousStates(S)))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetContinuousStates", "continuousStates[]", continuousStates))
        return fmi3Error;

    CALL(getContinuousStates(S, continuousStates, nContinuousStates));
#else
    UNUSED(continuousStates);
    UNUSED(nContinuousStates);
    return fmi3Error;
#endif

    END_FUNCTION();
}

fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Instance instance,
    fmi3Float64 nominals[],
    size_t nContinuousStates) {

    BEGIN_FUNCTION(GetNominalsOfContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi3GetNominalContinuousStates", "nContinuousStates", nContinuousStates, getNumberOfContinuousStates(instance)))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetNominalContinuousStates", "nominals[]", nominals))
        return fmi3Error;

    for (size_t i = 0; i < nContinuousStates; i++) {
        nominals[i] = 1;
    }

    return fmi3OK;
#else
    UNUSED(nominals);
    UNUSED(nContinuousStates);
    return fmi3Error;
#endif

    END_FUNCTION();
}

fmi3Status fmi3GetNumberOfEventIndicators(fmi3Instance instance,
    size_t* nEventIndicators) {

    BEGIN_FUNCTION(GetNumberOfEventIndicators);

    ASSERT_NOT_NULL(nEventIndicators);

#ifdef HAS_EVENT_INDICATORS
    *nEventIndicators = getNumberOfEventIndicators(instance);
#else
    *nEventIndicators = 0;
#endif


    END_FUNCTION();
}

fmi3Status fmi3GetNumberOfContinuousStates(fmi3Instance instance,
    size_t* nContinuousStates) {

    BEGIN_FUNCTION(GetNumberOfContinuousStates);

    ASSERT_NOT_NULL(nContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    *nContinuousStates = getNumberOfContinuousStates(instance);
#else
    *nContinuousStates = 0;
#endif

    END_FUNCTION();
}

/***************************************************
 Functions for Co-Simulation
 ****************************************************/

fmi3Status fmi3EnterStepMode(fmi3Instance instance) {

    BEGIN_FUNCTION(EnterStepMode);

    S->state = StepMode;

    END_FUNCTION();
}

fmi3Status fmi3GetOutputDerivatives(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int32 orders[],
    fmi3Float64 values[],
    size_t nValues) {

    UNUSED(nValues);

    BEGIN_FUNCTION(GetOutputDerivatives);

#ifdef GET_OUTPUT_DERIVATIVE
    for (size_t i = 0; i < nValueReferences; i++) {
        CALL(getOutputDerivative(S, (ValueReference)valueReferences[i], orders[i], &values[i]));
    }
#else
    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(orders);
    UNUSED(values);

    NOT_IMPLEMENTED;
#endif

    END_FUNCTION();
}

fmi3Status fmi3DoStep(fmi3Instance instance,
    fmi3Float64 currentCommunicationPoint,
    fmi3Float64 communicationStepSize,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* eventHandlingNeeded,
    fmi3Boolean* terminateSimulation,
    fmi3Boolean* earlyReturn,
    fmi3Float64* lastSuccessfulTime) {

    UNUSED(noSetFMUStatePriorToCurrentPoint);

    BEGIN_FUNCTION(DoStep);

    if (!isClose(currentCommunicationPoint, S->nextCommunicationPoint)) {
        logError(S, "Expected currentCommunicationPoint = %.16g but was %.16g.",
            S->nextCommunicationPoint, currentCommunicationPoint);
        CALL(Error);
    }

    if (communicationStepSize <= 0) {
        logError(S, "Communication step size must be > 0 but was %.16g.", communicationStepSize);
        CALL(Error);
    }

    const fmi3Float64 nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;

    if (nextCommunicationPoint > S->stopTime && !isClose(nextCommunicationPoint, S->stopTime)) {
        logError(S, "At communication point %.16g a step size of %.16g was requested but stop time is %.16g.",
            currentCommunicationPoint, communicationStepSize, S->stopTime);
        CALL(Error);
    }

    bool nextCommunicationPointReached;

    *eventHandlingNeeded = fmi3False;
    *terminateSimulation = fmi3False;
    *earlyReturn         = fmi3False;

    while (true) {

        const fmi3Float64 nextSolverStepTime = S->time + FIXED_SOLVER_STEP;

        nextCommunicationPointReached = nextSolverStepTime > nextCommunicationPoint && !isClose(nextSolverStepTime, nextCommunicationPoint);

        if (nextCommunicationPointReached || (*eventHandlingNeeded && S->earlyReturnAllowed)) {
            break;
        }

#ifdef EVENT_UPDATE
        if (*eventHandlingNeeded) {
            eventUpdate(S);
            *eventHandlingNeeded = fmi3False;
        }
#endif

        bool stateEvent, timeEvent;

        CALL(doFixedStep(S, &stateEvent, &timeEvent));

#ifdef EVENT_UPDATE
        if (stateEvent || timeEvent) {

            if (S->eventModeUsed) {
                *eventHandlingNeeded = fmi3True;
            } else {
                CALL(eventUpdate(S));
#ifdef HAS_EVENT_INDICATORS
                CALL(getEventIndicators(S, S->prez, S->nz));
#endif
            }

            if (S->earlyReturnAllowed) {
                break;
            }
        }
#endif

        if (S->terminateSimulation) {
            break;
        }
    }

    *terminateSimulation = S->terminateSimulation;
    *earlyReturn         = S->earlyReturnAllowed && !nextCommunicationPointReached;
    *lastSuccessfulTime  = S->time;

    if (nextCommunicationPointReached) {
        S->nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;
    } else {
        S->nextCommunicationPoint = S->time;
    }

    END_FUNCTION();
}

fmi3Status fmi3ActivateModelPartition(fmi3Instance instance,
    fmi3ValueReference clockReference,
    fmi3Float64 activationTime) {

    BEGIN_FUNCTION(ActivateModelPartition);

    CALL(activateModelPartition(S, (ValueReference)clockReference, activationTime));

    END_FUNCTION();
}
