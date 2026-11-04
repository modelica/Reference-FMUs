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
    size_t index = 0; \
    for (size_t i = 0; i < nValueReferences; i++) { \
        const Status status = get ## T(instance, (ValueReference)valueReferences[i], values, nValues, &index); \
        if (status != OK) { \
            return map_status(status); \
        } \
    } \
    return fmi3OK;
//BEGIN_FUNCTION(Get ## T); \
//if (nValueReferences == 0) goto TERMINATE; \
//ASSERT_NOT_NULL(valueReferences); \
//if (nValues > 0) ASSERT_NOT_NULL(values); \
//if (S->isDirtyValues) { \
//    CALL(calculateValues(S)); \
//    S->isDirtyValues = false; \
//} \
//size_t index = 0; \
//for (size_t i = 0; i < nValueReferences; i++) { \
//    CALL(get ## T(S, (ValueReference)valueReferences[i], values, nValues, &index)); \
//} \
//if (index != nValues) { \
//    logError(S, "Expected nValues = %zu but was %zu.", index, nValues); \
//    CALL(Error); \
//} \
//END_FUNCTION()

#define SET_VARIABLES(T) \
    size_t index = 0; \
    for (size_t i = 0; i < nValueReferences; i++) { \
        const Status status = set ## T(instance, (ValueReference)valueReferences[i], values, nValues, &index); \
        if (status != OK) { \
            return map_status(status); \
        } \
    } \
    return fmi3OK;
//BEGIN_FUNCTION(Set ## T); \
//if (nValueReferences == 0) goto TERMINATE; \
//ASSERT_NOT_NULL(valueReferences); \
//size_t index = 0; \
//for (size_t i = 0; i < nValueReferences; i++) { \
//    CALL(set ## T(S, (ValueReference)valueReferences[i], values, nValues, &index)); \
//} \
//if (nValueReferences > 0) S->isDirtyValues = true; \
//if (index != nValues) { \
//    logError(S, "Expected nValues = %zu but was %zu.", index, nValues); \
//    CALL(Error); \
//} \
//END_FUNCTION()

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Function calls allowed state masks for both Model-exchange and Co-simulation
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

#define NOT_IMPLEMENTED \
do { \
    ModelInstance *comp = (ModelInstance *)instance; \
    logError(comp, "Function is not implemented."); \
    return fmi3Error; \
} while (0)

//#define BEGIN_FUNCTION(F) \
//Status status = OK; \
//if (!instance) return fmi3Error; \
//ModelInstance *S = (ModelInstance *)instance; \
//if (!allowedState(S, MASK_fmi3##F, #F)) CALL(Error);
//
//#define END_FUNCTION() \
//TERMINATE: \
//    return (fmi3Status)status;
//
//static bool allowedState(ModelInstance *instance, int statesExpected, char *name) {
//
//    if (!instance) {
//        return false;
//    }
//
//    if (!(instance->state & statesExpected)) {
//        logError(instance, "fmi3%s: Illegal call sequence.", name);
//        return false;
//    }
//
//    return true;
//}

/***************************************************
 Common Functions
 ****************************************************/

const char* fmi3GetVersion(void) {
    return fmi3Version;
}

#define CALL2(s) \
    switch (s) { \
    case OK: return fmi3OK; \
    default: return fmi3Fatal; \
    }

#define CALL(s) \
    if (s != OK) { \
        return map_status(s); \
    }

fmi3Status fmi3SetDebugLogging(fmi3Instance instance,
                               fmi3Boolean loggingOn,
                               size_t nCategories,
                               const fmi3String categories[]) {
    //BEGIN_FUNCTION(SetDebugLogging);
    CALL2(setDebugLogging(instance, loggingOn, nCategories, categories));

    //END_FUNCTION();
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

    return createModelInstance(
        (loggerType)logMessage,
        NULL,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn,
        ModelExchange);
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

static fmi3Status map_status(Status status) {
    switch (status) {
    case OK: return fmi3OK;
    default: return fmi3Discard;
    }
}

fmi3Status fmi3EnterInitializationMode(fmi3Instance instance,
                                       fmi3Boolean toleranceDefined,
                                       fmi3Float64 tolerance,
                                       fmi3Float64 startTime,
                                       fmi3Boolean stopTimeDefined,
                                       fmi3Float64 stopTime) {

    Status status = setupExperiment(instance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);

    if (status != OK) {
        return map_status(status);
    }

    status = enterInitializationMode(instance);

    return map_status(status);
}

fmi3Status fmi3ExitInitializationMode(fmi3Instance instance) {
    const Status status = exitInitializationMode(instance);
    return map_status(status);
}

fmi3Status fmi3EnterEventMode(fmi3Instance instance) {
    const Status status = enterEventMode(instance);
    return map_status(status);
}

fmi3Status fmi3Terminate(fmi3Instance instance) {
    const Status status = terminate(instance);
    return map_status(status);
}

fmi3Status fmi3Reset(fmi3Instance instance) {
    const Status status = reset(instance);
    return map_status(status);
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
    NOT_IMPLEMENTED;
//    BEGIN_FUNCTION(GetBinary);
//
//    if (nValueReferences == 0) {
//        goto TERMINATE;
//    } else {
//        ASSERT_NOT_NULL(valueReferences);
//    }
//
//    if (nValues > 0) {
//        ASSERT_NOT_NULL(values);
//    }
//
//    if (S->isDirtyValues) {
//        CALL(calculateValues(S));
//        S->isDirtyValues = false;
//    }
//
//    size_t index = 0;
//
//    for (size_t i = 0; i < nValueReferences; i++) {
//        CALL(getBinary(S, (ValueReference)valueReferences[i], valueSizes, (const char**)values, nValues, &index));
//    }
//
//    END_FUNCTION();
}

fmi3Status fmi3GetClock(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Clock values[]) {
    NOT_IMPLEMENTED;
//    BEGIN_FUNCTION(GetClock);
//
//    for (size_t i = 0; i < nValueReferences; i++) {
//        CALL(getClock(instance, (ValueReference)valueReferences[i], &values[i]));
//    }
//
//    END_FUNCTION();
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
    NOT_IMPLEMENTED;
//
//    BEGIN_FUNCTION(SetBinary);
//
//    if (nValueReferences == 0) {
//        goto TERMINATE;
//    }
//
//    ASSERT_NOT_NULL(valueReferences);
//
//    size_t index = 0;
//
//    for (size_t i = 0; i < nValueReferences; i++) {
//        CALL(setBinary(S, (ValueReference)valueReferences[i], valueSizes, (const char* const*)values, nValues, &index));
//    }
//
//    if (nValueReferences > 0) {
//        S->isDirtyValues = true;
//    }
//
//    if (index != nValues) {
//        logError(S, "Expected nValues = %zu but was %zu.", index, nValues); \
//        CALL(Error);
//    }
//
//    END_FUNCTION();
}

fmi3Status fmi3SetClock(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Clock values[]) {
    NOT_IMPLEMENTED;
    //BEGIN_FUNCTION(SetClock);
//
//    for (size_t i = 0; i < nValueReferences; i++) {
//        if (values[i]) {
//            CALL(activateClock(instance, (ValueReference)valueReferences[i]));
//        }
//    }
//
//    END_FUNCTION();
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
    const Status status = getFMUState(instance, FMUState);
    return map_status(status);
}

fmi3Status fmi3SetFMUState(fmi3Instance instance, fmi3FMUState FMUState) {
    const Status status = setFMUState(instance, FMUState);
    return map_status(status);
}

fmi3Status fmi3FreeFMUState(fmi3Instance instance, fmi3FMUState* FMUState) {
    NOT_IMPLEMENTED;
}

fmi3Status fmi3SerializedFMUStateSize(fmi3Instance instance,
    fmi3FMUState  FMUState,
    size_t* size) {
    NOT_IMPLEMENTED;
}

fmi3Status fmi3SerializeFMUState(fmi3Instance instance,
    fmi3FMUState  FMUState,
    fmi3Byte serializedState[],
    size_t size) {
    const Status status = serializeFMUState(instance, FMUState, serializedState, size);
    return map_status(status);
}

fmi3Status fmi3DeserializeFMUState(fmi3Instance instance,
    const fmi3Byte serializedState[],
    size_t size,
    fmi3FMUState* FMUState) {
    const Status status = deserializeFMUState(instance, serializedState, size, FMUState);
    return map_status(status);
}

static size_t valueReferenceElementIndex(const fmi3ValueReference valueReferences[], size_t position) {
    size_t index = 0;
    for (size_t k = 0; k < position; k++) {
        if (valueReferences[k] == valueReferences[position]) {
            index++;
        }
    }
    return index;
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
    //NOT_IMPLEMENTED;
//
//    UNUSED(nSeed);
//    UNUSED(nSensitivity);
//
//    BEGIN_FUNCTION(GetDirectionalDerivative);
//
    // TODO: check value references
    // TODO: assert nUnknowns == nDeltaOfUnknowns
    // TODO: assert nKnowns == nDeltaKnowns

    for (size_t i = 0; i < nUnknowns; i++) {
        sensitivity[i] = 0;
        for (size_t j = 0; j < nKnowns; j++) {
            double partialDerivative = 0;
            const Status status = getPartialDerivative(instance,
                (ValueReference)unknowns[i],
                valueReferenceElementIndex(unknowns, i),
                (ValueReference)knowns[j],
                valueReferenceElementIndex(knowns, j),
                &partialDerivative);
            if (status != OK) {
                return map_status(status);
            }
            sensitivity[i] += partialDerivative * seed[j];
        }
    }
//
//    END_FUNCTION();
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

    for (size_t i = 0; i < nKnowns; i++) {

        sensitivity[i] = 0;
        
        for (size_t j = 0; j < nUnknowns; j++) {
            double partialDerivative = 0;
            
            const Status status = getPartialDerivative(
                instance,
                (ValueReference)unknowns[j],
                valueReferenceElementIndex(unknowns, j),
                (ValueReference)knowns[i],
                valueReferenceElementIndex(knowns, i),
                &partialDerivative
            );

            if (status != OK) {
                return map_status(status);
            }
            
            sensitivity[i] += partialDerivative * seed[j];
        }
    }

    return fmi3OK;
}

fmi3Status fmi3EnterConfigurationMode(fmi3Instance instance) {
    CALL(enterConfigurationMode(instance));
}

fmi3Status fmi3ExitConfigurationMode(fmi3Instance instance) {
    const Status status = exitConfigurationMode(instance);
    return map_status(status);
}

fmi3Status fmi3GetIntervalDecimal(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 intervals[],
    fmi3IntervalQualifier qualifiers[]) {
    NOT_IMPLEMENTED;

//    BEGIN_FUNCTION(GetIntervalDecimal);
//
//    // TODO: Check nValueReferences != nValues ?
//
//    for (size_t i = 0; i < nValueReferences; i++) {
//        CALL(getInterval(instance, (ValueReference)valueReferences[i], &intervals[i], (int*)&qualifiers[i]));
//    }
//
//    END_FUNCTION();
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
    const Status status = updateDiscreteStates(
        instance,
        discreteStatesNeedUpdate,
        terminateSimulation,
        nominalsOfContinuousStatesChanged,
        valuesOfContinuousStatesChanged,
        nextEventTimeDefined,
        nextEventTime
    );
    return map_status(status);
}

/***************************************************
 Functions for Model Exchange
 ****************************************************/

fmi3Status fmi3EnterContinuousTimeMode(fmi3Instance instance) {
    const Status status = enterContinuousTimeMode(instance);
    return map_status(status);
}

fmi3Status fmi3CompletedIntegratorStep(fmi3Instance instance,
    fmi3Boolean  noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* enterEventMode,
    fmi3Boolean* terminateSimulation) {
    const Status status = completedIntegratorStep(instance, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);
    return map_status(status);
}

/* Providing independent variables and re-initialization of caching */
fmi3Status fmi3SetTime(fmi3Instance instance, fmi3Float64 time) {
    const Status status = setTime(instance, time);
    return map_status(status);
}

fmi3Status fmi3SetContinuousStates(fmi3Instance instance,
    const fmi3Float64 continuousStates[],
    size_t nContinuousStates) {
    const Status status = setContinuousStates(instance, continuousStates, nContinuousStates);
    return map_status(status);
}

/* Evaluation of the model equations */
fmi3Status fmi3GetContinuousStateDerivatives(fmi3Instance instance,
    fmi3Float64 derivatives[],
    size_t nContinuousStates) {
    const Status status = getDerivatives(instance, derivatives, nContinuousStates);
    return map_status(status);
}

fmi3Status fmi3GetEventIndicators(fmi3Instance instance,
    fmi3Float64 eventIndicators[],
    size_t nEventIndicators) {
    const Status status = getEventIndicators(instance, eventIndicators, nEventIndicators);
    return map_status(status);
}

fmi3Status fmi3GetContinuousStates(fmi3Instance instance,
    fmi3Float64 continuousStates[],
    size_t nContinuousStates) {
    const Status status = getContinuousStates(instance, continuousStates, nContinuousStates);
    return map_status(status);
}

fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Instance instance,
    fmi3Float64 nominals[],
    size_t nContinuousStates) {
    const Status status = getNominalsOfContinuousStates(instance, nominals, nContinuousStates);
    return map_status(status);
}

fmi3Status fmi3GetNumberOfEventIndicators(fmi3Instance instance,
    size_t* nEventIndicators) {
    const Status status = getNumberOfEventIndicators(instance, nEventIndicators);
    return map_status(status);
}

fmi3Status fmi3GetNumberOfContinuousStates(fmi3Instance instance,
    size_t* nContinuousStates) {
    const Status status = getNumberOfContinuousStates(instance, nContinuousStates);
    return map_status(status);
}

/***************************************************
 Functions for Co-Simulation
 ****************************************************/

fmi3Status fmi3EnterStepMode(fmi3Instance instance) {
    const Status status = enterStepMode(instance);
    return map_status(status);
}

fmi3Status fmi3GetOutputDerivatives(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int32 orders[],
    fmi3Float64 values[],
    size_t nValues) {
    UNUSED(nValues);

#ifdef GET_OUTPUT_DERIVATIVE
    Status status = OK;

    for (size_t i = 0; i < nValueReferences; i++) {
        const Status status = getOutputDerivative(instance, (ValueReference)valueReferences[i], orders[i], &values[i]);
        if (status != OK) {
            break;
        }
    }

    return map_status(status);
#else
    UNUSED(valueReferences);
    UNUSED(nValueReferences);
    UNUSED(orders);
    UNUSED(values);

    NOT_IMPLEMENTED;
#endif
}

fmi3Status fmi3DoStep(fmi3Instance instance,
    fmi3Float64 currentCommunicationPoint,
    fmi3Float64 communicationStepSize,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* eventHandlingNeeded,
    fmi3Boolean* terminateSimulation,
    fmi3Boolean* earlyReturn,
    fmi3Float64* lastSuccessfulTime) {
    const Status status = doStep(
        instance,
        currentCommunicationPoint,
        communicationStepSize,
        noSetFMUStatePriorToCurrentPoint,
        eventHandlingNeeded,
        terminateSimulation,
        earlyReturn,
        lastSuccessfulTime
    );
    return map_status(status);
}

fmi3Status fmi3ActivateModelPartition(fmi3Instance instance,
    fmi3ValueReference clockReference,
    fmi3Float64 activationTime) {
    const Status status = activateModelPartition(instance, (ValueReference)clockReference, activationTime);
    return map_status(status);
}
