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

#define GET_VARIABLES(T) \
    size_t index = 0; \
    for (size_t i = 0; i < nValueReferences; i++) { \
        fmi3Status result = map_status(get ## T(instance, (ValueReference)valueReferences[i], values, nValues, &index)); \
        if (result != fmi3OK) { \
            return result; \
        } \
    } \
    return fmi3OK;

#define SET_VARIABLES(T) \
    size_t index = 0; \
    for (size_t i = 0; i < nValueReferences; i++) { \
        fmi3Status result = map_status(set ## T(instance, (ValueReference)valueReferences[i], values, nValues, &index)); \
        if (result != fmi3OK) { \
            return result; \
        } \
    } \
    return fmi3OK;

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

#define NOT_IMPLEMENTED \
do { \
    ModelInstance *comp = (ModelInstance *)instance; \
    logError(comp, "Function is not implemented."); \
    return fmi3Error; \
} while (0)

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
    return map_status(setDebugLogging(instance, loggingOn, nCategories, categories));
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

    return createModelExchangeInstance(
        (loggerType)logMessage,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn);
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

    return createCoSimulationInstance(
        (loggerType)logMessage,
        (intermediateUpdateType)intermediateUpdate,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn,
        earlyReturnAllowed,
        eventModeUsed);
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
    return createScheduledExecutionInstance(
        (loggerType)logMessage,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourcePath,
        loggingOn,
        clockUpdate,
        lockPreemption,
        unlockPreemption);
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

    fmi3Status result = map_status(setupExperiment(instance, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime));
    if (result != fmi3OK) {
        return result;
    }

    return map_status(enterInitializationMode(instance));
}

fmi3Status fmi3ExitInitializationMode(fmi3Instance instance) {
    return map_status(exitInitializationMode(instance));
}

fmi3Status fmi3EnterEventMode(fmi3Instance instance) {
    return map_status(enterEventMode(instance));
}

fmi3Status fmi3Terminate(fmi3Instance instance) {
    return map_status(terminate(instance));
}

fmi3Status fmi3Reset(fmi3Instance instance) {
    return map_status(reset(instance));
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
    size_t index = 0;
    for (size_t i = 0; i < nValueReferences; i++) {
        fmi3Status result = map_status(getBinary(instance, (ValueReference)valueReferences[i], valueSizes, (const char**)values, nValues, &index));
        if (result != fmi3OK) {
            return result;
        }
    }
    return fmi3OK;
}

fmi3Status fmi3GetClock(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Clock values[]) {
    for (size_t i = 0; i < nValueReferences; i++) {
        fmi3Status result = map_status(getClock(instance, (ValueReference)valueReferences[i], &values[i]));
        if (result != fmi3OK) {
            return result;
        }
    }
    return fmi3OK;
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
    size_t index = 0;
    for (size_t i = 0; i < nValueReferences; i++) {
        fmi3Status result = map_status(setBinary(instance, (ValueReference)valueReferences[i], valueSizes, (const char* const*)values, nValues, &index));
        if (result != fmi3OK) {
            return result;
        }
    }
    return fmi3OK;
}

fmi3Status fmi3SetClock(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Clock values[]) {
    for (size_t i = 0; i < nValueReferences; i++) {
        if (values[i]) {
            fmi3Status result = map_status(activateClock(instance, (ValueReference)valueReferences[i]));
            if (result != fmi3OK) {
                return result;
            }
        }
    }
    return fmi3OK;
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
    return map_status(getFMUState(instance, FMUState));
}

fmi3Status fmi3SetFMUState(fmi3Instance instance, fmi3FMUState FMUState) {
    return map_status(setFMUState(instance, FMUState));
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
    return map_status(serializeFMUState(instance, FMUState, serializedState, size));
}

fmi3Status fmi3DeserializeFMUState(fmi3Instance instance,
    const fmi3Byte serializedState[],
    size_t size,
    fmi3FMUState* FMUState) {
    return map_status(deserializeFMUState(instance, serializedState, size, FMUState));
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
            fmi3Status result = map_status(getPartialDerivative(instance,
                (ValueReference)unknowns[i],
                valueReferenceElementIndex(unknowns, i),
                (ValueReference)knowns[j],
                valueReferenceElementIndex(knowns, j),
                &partialDerivative));
            if (result != fmi3OK) {
                return result;
            }
            sensitivity[i] += partialDerivative * seed[j];
        }
    }

    return fmi3OK;
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
            
            fmi3Status result = map_status(getPartialDerivative(
                instance,
                (ValueReference)unknowns[j],
                valueReferenceElementIndex(unknowns, j),
                (ValueReference)knowns[i],
                valueReferenceElementIndex(knowns, i),
                &partialDerivative
            ));

            if (result != fmi3OK) {
                return result;
            }

            sensitivity[i] += partialDerivative * seed[j];
        }
    }

    return fmi3OK;
}

fmi3Status fmi3EnterConfigurationMode(fmi3Instance instance) {
    return map_status(enterConfigurationMode(instance));
}

fmi3Status fmi3ExitConfigurationMode(fmi3Instance instance) {
    return map_status(exitConfigurationMode(instance));
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
    return map_status(updateDiscreteStates(
        instance,
        discreteStatesNeedUpdate,
        terminateSimulation,
        nominalsOfContinuousStatesChanged,
        valuesOfContinuousStatesChanged,
        nextEventTimeDefined,
        nextEventTime
    ));
}

/***************************************************
 Functions for Model Exchange
 ****************************************************/

fmi3Status fmi3EnterContinuousTimeMode(fmi3Instance instance) {
    return map_status(enterContinuousTimeMode(instance));
}

fmi3Status fmi3CompletedIntegratorStep(fmi3Instance instance,
    fmi3Boolean  noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* enterEventMode,
    fmi3Boolean* terminateSimulation) {
    return map_status(completedIntegratorStep(instance, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation));
}

/* Providing independent variables and re-initialization of caching */
fmi3Status fmi3SetTime(fmi3Instance instance, fmi3Float64 time) {
    return map_status(setTime(instance, time));
}

fmi3Status fmi3SetContinuousStates(fmi3Instance instance,
    const fmi3Float64 continuousStates[],
    size_t nContinuousStates) {
    return map_status(setContinuousStates(instance, continuousStates, nContinuousStates));
}

/* Evaluation of the model equations */
fmi3Status fmi3GetContinuousStateDerivatives(fmi3Instance instance,
    fmi3Float64 derivatives[],
    size_t nContinuousStates) {
    return map_status(getDerivatives(instance, derivatives, nContinuousStates));
}

fmi3Status fmi3GetEventIndicators(fmi3Instance instance,
    fmi3Float64 eventIndicators[],
    size_t nEventIndicators) {
    return map_status(getEventIndicators(instance, eventIndicators, nEventIndicators));
}

fmi3Status fmi3GetContinuousStates(fmi3Instance instance,
    fmi3Float64 continuousStates[],
    size_t nContinuousStates) {
    return map_status(getContinuousStates(instance, continuousStates, nContinuousStates));
}

fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Instance instance,
    fmi3Float64 nominals[],
    size_t nContinuousStates) {
    return map_status(getNominalsOfContinuousStates(instance, nominals, nContinuousStates));
}

fmi3Status fmi3GetNumberOfEventIndicators(fmi3Instance instance,
    size_t* nEventIndicators) {
    return map_status(getNumberOfEventIndicators(instance, nEventIndicators));
}

fmi3Status fmi3GetNumberOfContinuousStates(fmi3Instance instance,
    size_t* nContinuousStates) {
    return map_status(getNumberOfContinuousStates(instance, nContinuousStates));
}

/***************************************************
 Functions for Co-Simulation
 ****************************************************/

fmi3Status fmi3EnterStepMode(fmi3Instance instance) {
    return map_status(enterStepMode(instance));
}

fmi3Status fmi3GetOutputDerivatives(fmi3Instance instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int32 orders[],
    fmi3Float64 values[],
    size_t nValues) {
    UNUSED(nValues);

#ifdef GET_OUTPUT_DERIVATIVE
    for (size_t i = 0; i < nValueReferences; i++) {
        fmi3Status result = map_status(getOutputDerivative(instance, (ValueReference)valueReferences[i], orders[i], &values[i]));
        if (result != fmi3OK) {
            return result;
        }
    }

    return fmi3OK;
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
    return map_status(doStep(
        instance,
        currentCommunicationPoint,
        communicationStepSize,
        noSetFMUStatePriorToCurrentPoint,
        eventHandlingNeeded,
        terminateSimulation,
        earlyReturn,
        lastSuccessfulTime
    ));
}

fmi3Status fmi3ActivateModelPartition(fmi3Instance instance,
    fmi3ValueReference clockReference,
    fmi3Float64 activationTime) {
    return map_status(activateModelPartition(instance, (ValueReference)clockReference, activationTime));
}
