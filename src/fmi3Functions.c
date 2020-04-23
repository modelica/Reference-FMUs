/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#if FMI_VERSION != 3
#error FMI_VERSION must be 3
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "config.h"
#include "model.h"
#include "slave.h"


// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#ifndef DISABLE_PREFIX
#define pasteA(a,b)     a ## b
#define pasteB(a,b)    pasteA(a,b)
#define fmi3_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi3Functions.h"

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Function calls allowed state masks for both Model-exchange and Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi3GetTypesPlatform         (modelStartAndEnd | modelInstantiated | modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelStepComplete | modelStepInProgress | modelStepFailed | modelStepCanceled | modelTerminated | modelError)
#define MASK_fmi3GetVersion               MASK_fmi3GetTypesPlatform
#define MASK_fmi3SetDebugLogging          (modelInstantiated | modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelStepComplete | modelStepInProgress | modelStepFailed | modelStepCanceled | modelTerminated | modelError)
#define MASK_fmi3Instantiate              (modelStartAndEnd)
#define MASK_fmi3FreeInstance             (modelInstantiated | modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelStepComplete | modelStepFailed | modelStepCanceled | modelTerminated | modelError)
#define MASK_fmi3SetupExperiment          modelInstantiated
#define MASK_fmi3EnterInitializationMode  modelInstantiated
#define MASK_fmi3ExitInitializationMode   modelInitializationMode
#define MASK_fmi3Reset                    MASK_fmi3FreeInstance
#define MASK_fmi3GetFloat64               (modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelStepComplete | modelStepFailed | modelStepCanceled | modelTerminated | modelError)
#define MASK_fmi3GetUInt16                MASK_fmi3GetFloat64
#define MASK_fmi3GetInt32                 MASK_fmi3GetFloat64
#define MASK_fmi3GetBoolean               MASK_fmi3GetFloat64
#define MASK_fmi3GetString                MASK_fmi3GetFloat64
#define MASK_fmi3SetFloat64               (modelInstantiated | modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelStepComplete)
#define MASK_fmi3SetInt32                 (modelInstantiated | modelInitializationMode | modelEventMode | modelStepComplete)
#define MASK_fmi3SetBoolean               MASK_fmi3SetInt32
#define MASK_fmi3SetString                MASK_fmi3SetInt32
#define MASK_fmi3SetBinary                MASK_fmi3SetInt32
#define MASK_fmi3GetFMUState              MASK_fmi3FreeInstance
#define MASK_fmi3SetFMUState              MASK_fmi3FreeInstance
#define MASK_fmi3FreeFMUState             MASK_fmi3FreeInstance
#define MASK_fmi3SerializedFMUStateSize   MASK_fmi3FreeInstance
#define MASK_fmi3SerializeFMUState        MASK_fmi3FreeInstance
#define MASK_fmi3DeSerializeFMUState      MASK_fmi3FreeInstance
#define MASK_fmi3GetDirectionalDerivative (modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelStepComplete | modelStepFailed | modelStepCanceled | modelTerminated | modelError)

// TODO: fix masks
#define MASK_fmi3EnterEventMode            (~0) // (modelEventMode | modelContinuousTimeMode)
#define MASK_fmi3Terminate                 (~0) // (modelEventMode | modelContinuousTimeMode | modelStepComplete | modelStepFailed)
#define MASK_fmi3GetClock                  (~0)
#define MASK_fmi3SetClock                  (~0)
#define MASK_fmi3ActivateModelPartition    (~0)
#define MASK_fmi3GetDoStepDiscardedStatus  (~0)
#define MASK_fmi3GetAdjointDerivative      (~0)
#define MASK_fmi3EnterStepMode             (~0)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Model-exchange
// ---------------------------------------------------------------------------
#define MASK_fmi3NewDiscreteStates             modelEventMode
#define MASK_fmi3EnterContinuousTimeMode       modelEventMode
#define MASK_fmi3CompletedIntegratorStep       modelContinuousTimeMode
#define MASK_fmi3SetTime                       (modelEventMode | modelContinuousTimeMode)
#define MASK_fmi3SetContinuousStates           modelContinuousTimeMode
#define MASK_fmi3GetEventIndicators            (modelInitializationMode | modelEventMode | modelContinuousTimeMode | modelTerminated | modelError)
#define MASK_fmi3GetContinuousStates           MASK_fmi3GetEventIndicators
#define MASK_fmi3GetDerivatives                (modelEventMode | modelContinuousTimeMode | modelTerminated | modelError)
#define MASK_fmi3GetNominalsOfContinuousStates ( modelInstantiated | modelEventMode | modelContinuousTimeMode | modelTerminated | modelError)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi3SetRealInputDerivatives  (modelInstantiated | modelInitializationMode | modelStepComplete)
#define MASK_fmi3GetRealOutputDerivatives (modelStepComplete | modelStepFailed | modelStepCanceled | modelTerminated | modelError)
#define MASK_fmi3DoStep                   modelStepComplete
#define MASK_fmi3CancelStep               modelStepInProgress
#define MASK_fmi3GetStatus                (modelStepComplete | modelStepInProgress | modelStepFailed | modelTerminated)
#define MASK_fmi3GetRealStatus            MASK_fmi3GetStatus
#define MASK_fmi3GetIntegerStatus         MASK_fmi3GetStatus
#define MASK_fmi3GetBooleanStatus         MASK_fmi3GetStatus
#define MASK_fmi3GetStringStatus          MASK_fmi3GetStatus

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

#define NOT_IMPLEMENTED ModelInstance *comp = (ModelInstance *)instance; \
    logError(comp, "Function is not implemented."); \
    return fmi3Error;

// shorthand to access the model instance
#define S ((ModelInstance *)instance)

static fmi3Status unsupportedFunction(fmi3Instance instance, const char *fName, int statesExpected) {
    
//    if (invalidState(S, fName, statesExpected))
//        return fmi3Error;
    
    logError(S, "%s: Function not implemented.", fName);
    
    return fmi3Error;
}

/***************************************************
 Common Functions
 ****************************************************/

const char* fmi3GetVersion() {
    return fmi3Version;
}

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


#define ASSERT_STATE(S) if(!allowedState(instance, MASK_fmi3##S, #S)) return fmi3Error;


fmi3Status fmi3SetDebugLogging(fmi3Instance instance, fmi3Boolean loggingOn, size_t nCategories, const fmi3String categories[]) {

    ASSERT_STATE(SetDebugLogging)

    return setDebugLogging(S, loggingOn, nCategories, categories);
}

fmi3Instance fmi3InstantiateModelExchange(
    fmi3String                 instanceName,
    fmi3String                 instantiationToken,
    fmi3String                 resourceLocation,
    fmi3Boolean                visible,
    fmi3Boolean                loggingOn,
    fmi3InstanceEnvironment    instanceEnvironment,
    fmi3CallbackLogMessage     logMessage) {
    
#ifndef MODEL_EXCHANGE
    return NULL;
#else
    return createModelInstance(
        (loggerType)logMessage,
        NULL,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        ModelExchange,
        false
    );
#endif
}

fmi3Instance fmi3InstantiateBasicCoSimulation(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourceLocation,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3Boolean                    intermediateVariableGetRequired,
    fmi3Boolean                    intermediateInternalVariableGetRequired,
    fmi3Boolean                    intermediateVariableSetRequired,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3CallbackLogMessage         logMessage,
    fmi3CallbackIntermediateUpdate intermediateUpdate) {

    return createModelInstance(
        (loggerType)logMessage,
        (intermediateUpdateType)intermediateUpdate,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        BasicCoSimulation,
        false
    );
}

fmi3Instance fmi3InstantiateHybridCoSimulation(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourceLocation,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3Boolean                    intermediateVariableGetRequired,
    fmi3Boolean                    intermediateInternalVariableGetRequired,
    fmi3Boolean                    intermediateVariableSetRequired,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3CallbackLogMessage         logMessage,
    fmi3CallbackIntermediateUpdate intermediateUpdate) {
    
    return NULL;  // not implemented
}

fmi3Instance fmi3InstantiateScheduledCoSimulation(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourceLocation,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3Boolean                    intermediateVariableGetRequired,
    fmi3Boolean                    intermediateInternalVariableGetRequired,
    fmi3Boolean                    intermediateVariableSetRequired,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3CallbackLogMessage         logMessage,
    fmi3CallbackIntermediateUpdate intermediateUpdate,
    fmi3CallbackLockPreemption     lockPreemption,
    fmi3CallbackUnlockPreemption   unlockPreemption) {
    
#ifndef SCHEDULED_CO_SIMULATION
    return NULL;
#else
    ModelInstance *instance = createModelInstance(
        (loggerType)logMessage,
        (intermediateUpdateType) intermediateUpdate,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        ScheduledCoSimulation,
        false
    );
    
    S->lockPreemtion = lockPreemption;
    S->unlockPreemtion = unlockPreemption;

    return instance;
#endif
}


void fmi3FreeInstance(fmi3Instance instance) {
    
    if (S) {
        freeModelInstance(S);
    }
}

fmi3Status fmi3EnterInitializationMode(fmi3Instance instance, fmi3Boolean toleranceDefined, fmi3Float64 tolerance, fmi3Float64 startTime, fmi3Boolean stopTimeDefined, fmi3Float64 stopTime) {
    
    ASSERT_STATE(EnterInitializationMode)
    
    S->state = modelInitializationMode;
    
    return fmi3OK;
}

fmi3Status fmi3ExitInitializationMode(fmi3Instance instance) {

    ASSERT_STATE(ExitInitializationMode)

    // if values were set and no fmi3GetXXX triggered update before,
    // ensure calculated values are updated now
    if (S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    if (S->type == ModelExchange) {
        S->state = modelEventMode;
        S->isNewEventIteration = true;
    } else {
        S->state = modelStepComplete;
    }

#if NUMBER_OF_EVENT_INDICATORS > 0
    // initialize event indicators
    getEventIndicators(S, S->prez, NUMBER_OF_EVENT_INDICATORS);
#endif

    return fmi3OK;
}

fmi3Status fmi3EnterEventMode(fmi3Instance instance,
                              fmi3Boolean inputEvent,
                              fmi3Boolean stepEvent,
                              const fmi3Int32 rootsFound[],
                              size_t nEventIndicators,
                              fmi3Boolean timeEvent) {
    
    ASSERT_STATE(EnterEventMode)

    S->state = modelEventMode;
    S->isNewEventIteration = true;
    
    return fmi3OK;
}

fmi3Status fmi3Terminate(fmi3Instance instance) {
    
    ASSERT_STATE(Terminate)
     
    S->state = modelTerminated;
    
    return fmi3OK;
}

fmi3Status fmi3Reset(fmi3Instance instance) {

    ASSERT_STATE(Reset)

    S->state = modelInstantiated;
    setStartValues(S);
    S->isDirtyValues = true;
    
    return fmi3OK;
}

fmi3Status fmi3GetFloat32(fmi3Instance instance,
                          const fmi3ValueReference valueReferences[], size_t nValueReferences,
                          fmi3Float32 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetFloat64(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, fmi3Float64 value[], size_t nValues) {

    ASSERT_STATE(GetFloat64)

    if (nvr > 0 && nullPointer(S, "fmi3GetReal", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(S, "fmi3GetReal", "value[]", value))
        return fmi3Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_VARIABLES(Float64)
}

fmi3Status fmi3GetInt8(fmi3Instance instance,
                       const fmi3ValueReference valueReferences[], size_t nValueReferences,
                       fmi3Int8 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetUInt8(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        fmi3UInt8 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetInt16(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        fmi3Int16 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetUInt16(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, fmi3UInt16 value[], size_t nValues) {
    
    ASSERT_STATE(GetUInt16)
    
    GET_VARIABLES(UInt16)
}

fmi3Status fmi3GetInt32(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, fmi3Int32 value[], size_t nValues) {

    ASSERT_STATE(GetInt32)

    if (nvr > 0 && nullPointer(S, "fmi3GetInteger", "vr[]", vr))
            return fmi3Error;

    if (nvr > 0 && nullPointer(S, "fmi3GetInteger", "value[]", value))
            return fmi3Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_VARIABLES(Int32)
}

fmi3Status fmi3GetUInt32(fmi3Instance instance,
                         const fmi3ValueReference valueReferences[], size_t nValueReferences,
                         fmi3UInt32 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetInt64(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        fmi3Int64 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetUInt64(fmi3Instance instance,
                         const fmi3ValueReference valueReferences[], size_t nValueReferences,
                         fmi3UInt64 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetBoolean(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, fmi3Boolean value[], size_t nValues) {

    ASSERT_STATE(GetBoolean)

    if (nvr > 0 && nullPointer(S, "fmi3GetBoolean", "vr[]", vr))
            return fmi3Error;

    if (nvr > 0 && nullPointer(S, "fmi3GetBoolean", "value[]", value))
            return fmi3Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_BOOLEAN_VARIABLES
}

fmi3Status fmi3GetString(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, fmi3String value[], size_t nValues) {

    ASSERT_STATE(GetBoolean)

    if (nvr>0 && nullPointer(S, "fmi3GetString", "vr[]", vr))
            return fmi3Error;

    if (nvr>0 && nullPointer(S, "fmi3GetString", "value[]", value))
            return fmi3Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_VARIABLES(String)
}

fmi3Status fmi3GetBinary(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, size_t size[], fmi3Binary value[], size_t nValues) {

    ASSERT_STATE(GetString)

    Status status = OK;

    for (int i = 0; i < nvr; i++) {
        size_t index = 0;
        Status s = getBinary(S, vr[i], size, value, &index);
        status = max(status, s);
        if (status > Warning) return status;
    }

    return status;
}

fmi3Status fmi3SetFloat32(fmi3Instance instance,
                          const fmi3ValueReference valueReferences[], size_t nValueReferences,
                          const fmi3Float32 values[], size_t nValues) {
    NOT_IMPLEMENTED
}


fmi3Status fmi3SetFloat64(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, const fmi3Float64 value[], size_t nValues) {

    ASSERT_STATE(SetFloat64)

    if (nvr > 0 && nullPointer(S, "fmi3SetReal", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(S, "fmi3SetReal", "value[]", value))
        return fmi3Error;

    SET_VARIABLES(Float64)
}

fmi3Status fmi3SetInt8(fmi3Instance instance,
                       const fmi3ValueReference valueReferences[], size_t nValueReferences,
                       const fmi3Int8 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetUInt8(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        const fmi3UInt8 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetInt16(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        const fmi3Int16 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetUInt16(fmi3Instance instance,
                         const fmi3ValueReference vr[], size_t nvr,
                         const fmi3UInt16 value[], size_t nValues) {

    SET_VARIABLES(UInt16)
}

fmi3Status fmi3SetInt32(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, const fmi3Int32 value[], size_t nValues) {
    
    ASSERT_STATE(SetInt32)

    if (nvr > 0 && nullPointer(S, "fmi3SetInteger", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(S, "fmi3SetInteger", "value[]", value))
        return fmi3Error;

    SET_VARIABLES(Int32)
}

fmi3Status fmi3SetUInt32(fmi3Instance instance,
                         const fmi3ValueReference valueReferences[], size_t nValueReferences,
                         const fmi3UInt32 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetInt64(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        const fmi3Int64 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetUInt64(fmi3Instance instance,
                         const fmi3ValueReference valueReferences[], size_t nValueReferences,
                         const fmi3UInt64 values[], size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetBoolean(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, const fmi3Boolean value[], size_t nValues) {

    ASSERT_STATE(SetBoolean)

    if (nvr > 0 && nullPointer(S, "fmi3SetBoolean", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(S, "fmi3SetBoolean", "value[]", value))
        return fmi3Error;

    SET_BOOLEAN_VARIABLES
}

fmi3Status fmi3SetString(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, const fmi3String value[], size_t nValues) {

    ASSERT_STATE(SetString)

    if (nvr>0 && nullPointer(S, "fmi3SetString", "vr[]", vr))
        return fmi3Error;

    if (nvr>0 && nullPointer(S, "fmi3SetString", "value[]", value))
        return fmi3Error;

    SET_VARIABLES(String)
}

fmi3Status fmi3SetBinary(fmi3Instance instance, const fmi3ValueReference vr[], size_t nvr, const size_t size[], const fmi3Binary value[], size_t nValues) {

    ASSERT_STATE(SetBinary)

    Status status = OK;

    for (int i = 0; i < nvr; i++) {
        size_t index = 0;
        Status s = setBinary(S, vr[i], size, value, &index);
        status = max(status, s);
        if (status > Warning) return status;
    }

    return status;
}

fmi3Status fmi3GetNumberOfVariableDependencies(fmi3Instance instance,
                                               fmi3ValueReference valueReference,
                                               size_t* nDependencies) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetVariableDependencies(fmi3Instance instance,
                                       fmi3ValueReference dependent,
                                       size_t elementIndicesOfDependent[],
                                       fmi3ValueReference independents[],
                                       size_t elementIndicesOfIndependents[],
                                       fmi3DependencyKind dependencyKinds[],
                                       size_t nDependencies) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetFMUState(fmi3Instance instance, fmi3FMUState* FMUState) {
    return unsupportedFunction(instance, "fmi3GetFMUState", MASK_fmi3GetFMUState);
}
fmi3Status fmi3SetFMUState(fmi3Instance instance, fmi3FMUState FMUState) {
    return unsupportedFunction(instance, "fmi3SetFMUState", MASK_fmi3SetFMUState);
}
fmi3Status fmi3FreeFMUState(fmi3Instance instance, fmi3FMUState* FMUState) {
    return unsupportedFunction(instance, "fmi3FreeFMUState", MASK_fmi3FreeFMUState);
}
fmi3Status fmi3SerializedFMUStateSize(fmi3Instance instance, fmi3FMUState FMUState, size_t *size) {
    return unsupportedFunction(instance, "fmi3SerializedFMUStateSize", MASK_fmi3SerializedFMUStateSize);
}
fmi3Status fmi3SerializeFMUState(fmi3Instance instance, fmi3FMUState FMUState, fmi3Byte serializedState[], size_t size) {
    return unsupportedFunction(instance, "fmi3SerializeFMUState", MASK_fmi3SerializeFMUState);
}
fmi3Status fmi3DeSerializeFMUState (fmi3Instance instance, const fmi3Byte serializedState[], size_t size,
                                    fmi3FMUState* FMUState) {
    return unsupportedFunction(instance, "fmi3DeSerializeFMUState", MASK_fmi3DeSerializeFMUState);
}

fmi3Status fmi3GetDirectionalDerivative(fmi3Instance instance, const fmi3ValueReference unknowns[], size_t nUnknowns, const fmi3ValueReference knowns[], size_t nKnowns, const fmi3Float64 deltaKnowns[], size_t nDeltaKnowns, fmi3Float64 deltaUnknowns[], size_t nDeltaOfUnknowns) {
    
    ASSERT_STATE(GetDirectionalDerivative)

    // TODO: check value references
    // TODO: assert nUnknowns == nDeltaOfUnknowns
    // TODO: assert nKnowns == nDeltaKnowns

    Status status = OK;

    for (int i = 0; i < nUnknowns; i++) {
        deltaUnknowns[i] = 0;
        for (int j = 0; j < nKnowns; j++) {
            double partialDerivative = 0;
            Status s = getPartialDerivative(S, unknowns[i], knowns[j], &partialDerivative);
            status = max(status, s);
            if (status > Warning) return status;
            deltaUnknowns[i] += partialDerivative * deltaKnowns[j];
        }
    }

    return fmi3OK;
}

fmi3Status fmi3GetAdjointDerivative(fmi3Instance instance,
    const fmi3ValueReference unknowns[],
    size_t nUnknowns,
    const fmi3ValueReference knowns[],
    size_t nKnowns,
    const fmi3Float64 deltaUnknowns[],
    size_t nDeltaOfUnknowns,
    fmi3Float64 deltaKnowns[],
    size_t nDeltaKnowns) {
    
    ASSERT_STATE(GetAdjointDerivative)

    // TODO: check value references
    
    Status status = OK;

    for (int i = 0; i < nKnowns; i++) {
        deltaKnowns[i] = 0;
        for (int j = 0; j < nUnknowns; j++) {
            double partialDerivative = 0;
            Status s = getPartialDerivative(S, unknowns[j], knowns[i], &partialDerivative);
            status = max(status, s);
            if (status > Warning) return status;
            deltaKnowns[i] += partialDerivative * deltaUnknowns[j];
        }
    }

    return fmi3OK;
}

fmi3Status fmi3EnterConfigurationMode(fmi3Instance instance) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3ExitConfigurationMode(fmi3Instance instance) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetClock(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        const fmi3Boolean value[], const fmi3Boolean *subactive) {
    
    ASSERT_STATE(SetClock)

    Status status = OK;

    for (size_t i = 0; i < nValueReferences; i++) {
        if (value[i]) {
            Status s = activateClock(instance,  valueReferences[i]);
            status = max(status, s);
            if (status > Warning) return status;
        }
    }

    return status;
}

fmi3Status fmi3GetClock(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[], size_t nValueReferences,
                        fmi3Clock value[]) {
    
    ASSERT_STATE(GetClock)

    Status status = OK;

    for (size_t i = 0; i < nValueReferences; i++) {
        Status s = getClock(instance, valueReferences[i], &value[i]);
        status = max(status, s);
        if (status > Warning) return status;
    }

    return status;
}

fmi3Status fmi3GetIntervalDecimal(fmi3Instance instance,
                                      const fmi3ValueReference valueReferences[], size_t nValueReferences,
                                  fmi3Float64 interval[]) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetIntervalDecimal(fmi3Instance instance,
                                  const fmi3ValueReference valueReferences[],
                                  size_t nValueReferences,
                                  const fmi3Float64 interval[]) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetIntervalFraction(fmi3Instance instance,
                                   const fmi3ValueReference valueReferences[],
                                   size_t nValueReferences,
                                   fmi3UInt64 intervalCounter[],
                                   fmi3UInt64 resolution[]) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetIntervalFraction(fmi3Instance instance,
                                   const fmi3ValueReference valueReferences[],
                                   size_t nValueReferences,
                                   const fmi3UInt64 intervalCounter[],
                                   const fmi3UInt64 resolution[]) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3NewDiscreteStates(fmi3Instance instance,
                                 fmi3Boolean *newDiscreteStatesNeeded,
                                 fmi3Boolean *terminateSimulation,
                                 fmi3Boolean *nominalsOfContinuousStatesChanged,
                                 fmi3Boolean *valuesOfContinuousStatesChanged,
                                 fmi3Boolean *nextEventTimeDefined,
                                 fmi3Float64 *nextEventTime) {
    
    ASSERT_STATE(NewDiscreteStates)

    S->newDiscreteStatesNeeded           = false;
    S->terminateSimulation               = false;
    S->nominalsOfContinuousStatesChanged = false;
    S->valuesOfContinuousStatesChanged   = false;

    eventUpdate(S);

    S->isNewEventIteration = false;

    // copy internal eventInfo of component to output arguments
    *newDiscreteStatesNeeded           = S->newDiscreteStatesNeeded;
    *terminateSimulation               = S->terminateSimulation;
    *nominalsOfContinuousStatesChanged = S->nominalsOfContinuousStatesChanged;
    *valuesOfContinuousStatesChanged   = S->valuesOfContinuousStatesChanged;
    *nextEventTimeDefined              = S->nextEventTimeDefined;
    *nextEventTime                     = S->nextEventTime;

    return fmi3OK;
}

/***************************************************
 Functions for FMI3 for Model Exchange
 ****************************************************/

fmi3Status fmi3EnterContinuousTimeMode(fmi3Instance instance) {
    
    ASSERT_STATE(EnterContinuousTimeMode)

    S->state = modelContinuousTimeMode;

    return fmi3OK;
}

fmi3Status fmi3CompletedIntegratorStep(fmi3Instance instance, fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                                       fmi3Boolean *enterEventMode, fmi3Boolean *terminateSimulation) {
    
    ASSERT_STATE(CompletedIntegratorStep)

    if (nullPointer(S, "fmi3CompletedIntegratorStep", "enterEventMode", enterEventMode))
        return fmi3Error;

    if (nullPointer(S, "fmi3CompletedIntegratorStep", "terminateSimulation", terminateSimulation))
        return fmi3Error;

    *enterEventMode = fmi3False;
    *terminateSimulation = fmi3False;

    return fmi3OK;
}

/* Providing independent variables and re-initialization of caching */
fmi3Status fmi3SetTime(fmi3Instance instance, fmi3Float64 time) {
    
    ASSERT_STATE(SetTime)

    S->time = time;

    return fmi3OK;
}

fmi3Status fmi3SetContinuousStates(fmi3Instance instance, const fmi3Float64 x[], size_t nx){

    ASSERT_STATE(SetContinuousStates)

    if (invalidNumber(S, "fmi3SetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(S, "fmi3SetContinuousStates", "x[]", x))
        return fmi3Error;

    setContinuousStates(S, x, nx);

    return fmi3OK;
}

/* Evaluation of the model equations */
fmi3Status fmi3GetDerivatives(fmi3Instance instance, fmi3Float64 derivatives[], size_t nx) {

    ASSERT_STATE(GetDerivatives)

    if (invalidNumber(S, "fmi3GetDerivatives", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetDerivatives", "derivatives[]", derivatives))
        return fmi3Error;

    getDerivatives(S, derivatives, nx);

    return fmi3OK;
}

fmi3Status fmi3GetEventIndicators(fmi3Instance instance, fmi3Float64 eventIndicators[], size_t ni) {

    ASSERT_STATE(GetEventIndicators)

#if NUMBER_OF_EVENT_INDICATORS > 0
    if (invalidNumber(S, "fmi3GetEventIndicators", "ni", ni, NUMBER_OF_EVENT_INDICATORS))
        return fmi3Error;

    getEventIndicators(S, eventIndicators, ni);
#else
    if (ni > 0) return fmi3Error;
#endif
    return fmi3OK;
}

fmi3Status fmi3GetContinuousStates(fmi3Instance instance, fmi3Float64 states[], size_t nx) {

    ASSERT_STATE(GetContinuousStates)

    if (invalidNumber(S, "fmi3GetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetContinuousStates", "states[]", states))
        return fmi3Error;

    getContinuousStates(S, states, nx);

    return fmi3OK;
}

fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Instance instance, fmi3Float64 x_nominal[], size_t nx) {
    
    ASSERT_STATE(GetNominalsOfContinuousStates)

    if (invalidNumber(S, "fmi3GetNominalContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetNominalContinuousStates", "x_nominal[]", x_nominal))
        return fmi3Error;

    for (size_t i = 0; i < nx; i++) {
        x_nominal[i] = 1;
    }
    
    return fmi3OK;
}

fmi3Status fmi3GetNumberOfEventIndicators(fmi3Instance instance, size_t* nz) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetNumberOfContinuousStates(fmi3Instance instance, size_t* nx) {
    NOT_IMPLEMENTED
}

/***************************************************
 Functions for FMI3 for Co-Simulation
 ****************************************************/

fmi3Status fmi3EnterStepMode(fmi3Instance instance) {
    
    ASSERT_STATE(EnterStepMode)

    return fmi3OK;
}

fmi3Status fmi3SetInputDerivatives(fmi3Instance instance,
                                   const fmi3ValueReference valueReferences[],
                                   size_t nValueReferences,
                                   const fmi3Int32 orders[],
                                   const fmi3Float64 values[],
                                   size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetOutputDerivatives(fmi3Instance instance,
                                    const fmi3ValueReference valueReferences[],
                                    size_t nValueReferences,
                                    const fmi3Int32 orders[],
                                    fmi3Float64 values[],
                                    size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3DoStep(fmi3Instance instance,
                      fmi3Float64 currentCommunicationPoint,
                      fmi3Float64 communicationStepSize,
                      fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                      fmi3Boolean* earlyReturn) {

    ASSERT_STATE(DoStep)

    if (communicationStepSize <= 0) {
        logError(S, "fmi3DoStep: communication step size must be > 0 but was %g.", communicationStepSize);
        S->state = modelError;
        return fmi3Error;
    }

    return doStep(S, currentCommunicationPoint, currentCommunicationPoint + communicationStepSize, earlyReturn);
}

fmi3Status fmi3ActivateModelPartition(fmi3Instance instance,
                                      fmi3ValueReference clockReference,
                                      fmi3Float64 activationTime) {
    
    ASSERT_STATE(ActivateModelPartition)
    
    return activateModelPartition(S, clockReference, activationTime);
}

fmi3Status fmi3DoEarlyReturn(fmi3Instance instance, fmi3Float64 earlyReturnTime) {
    
    ASSERT_STATE(ActivateModelPartition)

    S->returnEarly = true;

    return fmi3OK;
}

fmi3Status fmi3GetDoStepDiscardedStatus(fmi3Instance instance, fmi3Boolean* terminate, fmi3Float64* lastSuccessfulTime) {

    ASSERT_STATE(GetDoStepDiscardedStatus)

    return fmi3Error;
}
