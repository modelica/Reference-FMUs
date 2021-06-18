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
#include <stdlib.h>  // for calloc(), free()
#include <float.h>   // for DBL_EPSILON
#include <math.h>    // for fabs()

#include "config.h"
#include "model.h"
#include "cosimulation.h"

#define FMI_STATUS fmi3Status

// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#if !defined(DISABLE_PREFIX) && !defined(FMI3_FUNCTION_PREFIX)
#define pasteA(a,b)          a ## b
#define pasteB(a,b)          pasteA(a,b)
#define FMI3_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi3Functions.h"

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
#define MASK_fmi3GetFloat32               (InitializationMode | ConfigurationMode | ReconfigurationMode | EventMode | ContinuousTimeMode | StepMode | ClockActivationMode | IntermediateUpdateMode | Terminated)
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

/* Getting Variable Dependency Information */
#define MASK_fmi3GetNumberOfVariableDependencies  MASK_AnyState
#define MASK_fmi3GetVariableDependencies          MASK_AnyState

/* Getting and setting the internal FMU state */
#define MASK_fmi3GetFMUState              MASK_AnyState
#define MASK_fmi3SetFMUState              MASK_AnyState
#define MASK_fmi3FreeFMUState             MASK_AnyState
#define MASK_fmi3SerializedFMUStateSize   MASK_AnyState
#define MASK_fmi3SerializeFMUState        MASK_AnyState
#define MASK_fmi3DeSerializeFMUState      MASK_AnyState

/* Getting partial derivatives */
#define MASK_fmi3GetDirectionalDerivative (InitializationMode | EventMode | ContinuousTimeMode | Terminated)
#define MASK_fmi3GetAdjointDerivative     MASK_fmi3GetDirectionalDerivative

/* Entering and exiting the Configuration or Reconfiguration Mode */
#define MASK_fmi3EnterConfigurationMode   (Instantiated | StepMode | EventMode | ClockActivationMode)
#define MASK_fmi3ExitConfigurationMode    (ConfigurationMode | ReconfigurationMode)

/* Clock related functions */
// TODO: fix masks
#define MASK_fmi3GetClock                  MASK_AnyState
#define MASK_fmi3SetClock                  MASK_AnyState
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
#define MASK_fmi3GetNumberOfEventIndicators    MASK_fmi3GetContinuousStateDerivatives
#define MASK_fmi3GetNumberOfContinuousStates   MASK_fmi3GetContinuousStateDerivatives

/* Functions for Co-Simulation */

#define MASK_fmi3EnterStepMode            (InitializationMode | EventMode)
#define MASK_fmi3SetInputDerivatives      (Instantiated | InitializationMode | StepMode)
#define MASK_fmi3GetOutputDerivatives     (StepMode | StepDiscarded | Terminated | Error)
#define MASK_fmi3DoStep                   StepMode
#define MASK_fmi3ActivateModelPartition   ClockActivationMode
#define MASK_fmi3DoEarlyReturn            IntermediateUpdateMode
#define MASK_fmi3GetDoStepDiscardedStatus StepMode

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

#define NOT_IMPLEMENTED ModelInstance *comp = (ModelInstance *)instance; \
    logError(comp, "Function is not implemented."); \
    return fmi3Error;

// shorthand to access the model instance
#define S ((ModelInstance *)instance)

/***************************************************
 Common Functions
 ****************************************************/

const char* fmi3GetVersion() {
    return fmi3Version;
}

#define ASSERT_STATE(S) if(!allowedState(instance, MASK_fmi3##S, #S)) return fmi3Error;

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

fmi3Status fmi3SetDebugLogging(fmi3Instance instance, fmi3Boolean loggingOn, size_t nCategories, const fmi3String categories[]) {

    ASSERT_STATE(SetDebugLogging)

    return (fmi3Status)setDebugLogging(S, loggingOn, nCategories, categories);
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

fmi3Instance fmi3InstantiateCoSimulation(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourceLocation,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3Boolean                    eventModeUsed,
    fmi3Boolean                    earlyReturnAllowed,
    const fmi3ValueReference       requiredIntermediateVariables[],
    size_t                         nRequiredIntermediateVariables,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3CallbackLogMessage         logMessage,
    fmi3CallbackIntermediateUpdate intermediateUpdate) {

    ModelInstance *instance = createModelInstance(
        (loggerType)logMessage,
        (intermediateUpdateType)intermediateUpdate,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        CoSimulation,
        false
    );

    if (instance) {
        instance->state = Instantiated;
    }

    return instance;
}

fmi3Instance fmi3InstantiateScheduledExecution(
    fmi3String                     instanceName,
    fmi3String                     instantiationToken,
    fmi3String                     resourceLocation,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    const fmi3ValueReference       requiredIntermediateVariables[],
    size_t                         nRequiredIntermediateVariables,
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
        (intermediateUpdateType)intermediateUpdate,
        instanceEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        ScheduledExecution,
        false
    );

    if (instance) {
        instance->state = Instantiated;
        instance->lockPreemtion = lockPreemption;
        instance->unlockPreemtion = unlockPreemption;
    }

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

    S->state = InitializationMode;

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

    switch (S->type) {
        case ModelExchange:
            S->state = EventMode;
            S->isNewEventIteration = true;
            break;
        case CoSimulation:
            S->state = StepMode;
            // TODO: new event iteration?
            break;
        case ScheduledExecution:
            S->state = ClockActivationMode;
            break;
    }

#if NZ > 0
    // initialize event indicators
    getEventIndicators(S, S->prez, NZ);
#endif

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

    return fmi3OK;
}

fmi3Status fmi3EnterEventMode(fmi3Instance instance,
    fmi3Boolean stepEvent,
    fmi3Boolean stateEvent,
    const fmi3Int32 rootsFound[],
    size_t nEventIndicators,
    fmi3Boolean timeEvent) {

    ASSERT_STATE(EnterEventMode)

    S->state = EventMode;
    S->isNewEventIteration = true;

    return fmi3OK;
}

fmi3Status fmi3Terminate(fmi3Instance instance) {

    ASSERT_STATE(Terminate)

    S->state = Terminated;

    return fmi3OK;
}

fmi3Status fmi3Reset(fmi3Instance instance) {

    ASSERT_STATE(Reset)

    S->state = Instantiated;
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

    for (int i = 0; i < nvr; i++) {
        if (vr[i] == 0) {
            value[i] = S->time;
            break;
        }
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
        Status s = getBinary(S, vr[i], size, (const char**)value, &index);
        status = max(status, s);
        if (status > Warning) return (fmi3Status)status;
    }

    return (fmi3Status)status;
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
        Status s = setBinary(S, vr[i], size, (const char* const*)value, &index);
        status = max(status, s);
        if (status > Warning) return (fmi3Status)status;
    }

    return (fmi3Status)status;
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

    ASSERT_STATE(GetFMUState)

    ModelData *modelData = (ModelData *)calloc(1, sizeof(ModelData));
    memcpy(modelData, S->modelData, sizeof(ModelData));
    *FMUState = modelData;

    return fmi3OK;
}

fmi3Status fmi3SetFMUState(fmi3Instance instance, fmi3FMUState FMUState) {

    ASSERT_STATE(SetFMUState)

    ModelData *modelData = FMUState;
    memcpy(S->modelData, modelData, sizeof(ModelData));

    return fmi3OK;
}

fmi3Status fmi3FreeFMUState(fmi3Instance instance, fmi3FMUState* FMUState) {

    ASSERT_STATE(FreeFMUState)

    ModelData *modelData = *FMUState;
    free(modelData);
    *FMUState = NULL;

    return fmi3OK;
}

fmi3Status fmi3SerializedFMUStateSize(fmi3Instance instance, fmi3FMUState FMUState, size_t *size) {

     UNUSED(instance)
     UNUSED(FMUState)
     ASSERT_STATE(SerializedFMUStateSize)

     *size = sizeof(ModelData);

     return fmi3OK;
}

fmi3Status fmi3SerializeFMUState(fmi3Instance instance, fmi3FMUState FMUState, fmi3Byte serializedState[], size_t size) {

    ASSERT_STATE(SerializeFMUState)

    if (nullPointer(S, "fmi3SerializeFMUState", "FMUstate", FMUState)) {
        return fmi3Error;
    }

    if (invalidNumber(S, "fmi3SerializeFMUState", "size", size, sizeof(ModelData))) {
        return fmi3Error;
    }

    memcpy(serializedState, FMUState, sizeof(ModelData));

    return fmi3OK;
}

fmi3Status fmi3DeSerializeFMUState (fmi3Instance instance, const fmi3Byte serializedState[], size_t size,
                                    fmi3FMUState* FMUState) {
    ASSERT_STATE(DeSerializeFMUState)

    if (*FMUState == NULL) {
        *FMUState = (fmi3FMUState *)calloc(1, sizeof(ModelData));
    }

    if (invalidNumber(S, "fmi3DeSerializeFMUState", "size", size, sizeof(ModelData)))
        return fmi3Error;

    memcpy(*FMUState, serializedState, sizeof(ModelData));

    return fmi3OK;
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
            if (status > Warning) return (fmi3Status)status;
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
            if (status > Warning) return (fmi3Status)status;
            deltaKnowns[i] += partialDerivative * deltaUnknowns[j];
        }
    }

    return fmi3OK;
}

fmi3Status fmi3EnterConfigurationMode(fmi3Instance instance) {

    ASSERT_STATE(EnterConfigurationMode)

    S->state = (S->state == Instantiated) ? ConfigurationMode : ReconfigurationMode;

    return fmi3OK;
}

fmi3Status fmi3ExitConfigurationMode(fmi3Instance instance) {

    ASSERT_STATE(ExitConfigurationMode)

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

    return fmi3OK;
}

fmi3Status fmi3SetClock(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[],
                        size_t nValueReferences,
                        const fmi3Clock values[],
                        size_t nValues) {

    ASSERT_STATE(SetClock)

    Status status = OK;

    for (size_t i = 0; i < nValueReferences; i++) {
        if (values[i]) {
            Status s = activateClock(instance,  valueReferences[i]);
            status = max(status, s);
            if (status > Warning) return (fmi3Status)status;
        }
    }

    return (fmi3Status)status;
}

fmi3Status fmi3GetClock(fmi3Instance instance,
                        const fmi3ValueReference valueReferences[],
                        size_t nValueReferences,
                        fmi3Clock values[],
                        size_t nValues) {

    ASSERT_STATE(GetClock)

    Status status = OK;

    for (size_t i = 0; i < nValueReferences; i++) {
        Status s = getClock(instance, valueReferences[i], &values[i]);
        status = max(status, s);
        if (status > Warning) return (fmi3Status)status;
    }

    return (fmi3Status)status;
}

fmi3Status fmi3GetIntervalDecimal(fmi3Instance instance,
                                  const fmi3ValueReference valueReferences[],
                                  size_t nValueReferences,
                                  fmi3Float64 interval[],
                                  fmi3IntervalQualifier qualifier[],
                                  size_t nValues) {

    // ? Check nValueReferences != nValues
    Status status = OK;

    for (size_t i = 0; i < nValueReferences; i++) {
        Status s = getInterval(instance, valueReferences[i], &interval[i], (int*)&qualifier[i]);
        status = max(status, s);
        if (status > Warning) return (fmi3Status)status;
    }

    return (fmi3Status)status;
}

fmi3Status fmi3SetIntervalDecimal(fmi3Instance instance,
                                  const fmi3ValueReference valueReferences[],
                                  size_t nValueReferences,
                                  const fmi3Float64 interval[],
                                  size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3GetIntervalFraction(fmi3Instance instance,
                                   const fmi3ValueReference valueReferences[],
                                   size_t nValueReferences,
                                   fmi3UInt64 intervalCounter[],
                                   fmi3UInt64 resolution[],
                                   fmi3IntervalQualifier qualifier[],
                                   size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3SetIntervalFraction(fmi3Instance instance,
                                   const fmi3ValueReference valueReferences[],
                                   size_t nValueReferences,
                                   const fmi3UInt64 intervalCounter[],
                                   const fmi3UInt64 resolution[],
                                   size_t nValues) {
    NOT_IMPLEMENTED
}

fmi3Status fmi3UpdateDiscreteStates(fmi3Instance instance,
                                    fmi3Boolean* discreteStatesNeedUpdate,
                                    fmi3Boolean* terminateSimulation,
                                    fmi3Boolean* nominalsOfContinuousStatesChanged,
                                    fmi3Boolean* valuesOfContinuousStatesChanged,
                                    fmi3Boolean* nextEventTimeDefined,
                                    fmi3Float64* nextEventTime) {

    ASSERT_STATE(NewDiscreteStates)

    S->newDiscreteStatesNeeded           = false;
    S->terminateSimulation               = false;
    S->nominalsOfContinuousStatesChanged = false;
    S->valuesOfContinuousStatesChanged   = false;

    eventUpdate(S);

    S->isNewEventIteration = false;

    // copy internal eventInfo of component to output arguments
    if (discreteStatesNeedUpdate)          *discreteStatesNeedUpdate          = S->newDiscreteStatesNeeded;
    if (terminateSimulation)               *terminateSimulation               = S->terminateSimulation;
    if (nominalsOfContinuousStatesChanged) *nominalsOfContinuousStatesChanged = S->nominalsOfContinuousStatesChanged;
    if (valuesOfContinuousStatesChanged)   *valuesOfContinuousStatesChanged   = S->valuesOfContinuousStatesChanged;
    if (nextEventTimeDefined)              *nextEventTimeDefined              = S->nextEventTimeDefined;
    if (nextEventTime)                     *nextEventTime                     = S->nextEventTime;

    return fmi3OK;
}

/***************************************************
 Functions for FMI3 for Model Exchange
 ****************************************************/

fmi3Status fmi3EnterContinuousTimeMode(fmi3Instance instance) {

    ASSERT_STATE(EnterContinuousTimeMode)

    S->state = ContinuousTimeMode;

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

    if (invalidNumber(S, "fmi3SetContinuousStates", "nx", nx, NX))
        return fmi3Error;

    if (nullPointer(S, "fmi3SetContinuousStates", "x[]", x))
        return fmi3Error;

    setContinuousStates(S, x, nx);

    return fmi3OK;
}

/* Evaluation of the model equations */
fmi3Status fmi3GetContinuousStateDerivatives(fmi3Instance instance, fmi3Float64 derivatives[], size_t nContinuousStates) {

    ASSERT_STATE(GetContinuousStateDerivatives)

    if (invalidNumber(S, "fmi3GetContinuousStateDerivatives", "nContinuousStates", nContinuousStates, NX))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetContinuousStateDerivatives", "derivatives[]", derivatives))
        return fmi3Error;

    getDerivatives(S, derivatives, nContinuousStates);

    return fmi3OK;
}

fmi3Status fmi3GetEventIndicators(fmi3Instance instance, fmi3Float64 eventIndicators[], size_t ni) {

    ASSERT_STATE(GetEventIndicators)

#if NZ > 0
    if (invalidNumber(S, "fmi3GetEventIndicators", "ni", ni, NZ))
        return fmi3Error;

    getEventIndicators(S, eventIndicators, ni);
#else
    if (ni > 0) return fmi3Error;
#endif
    return fmi3OK;
}

fmi3Status fmi3GetContinuousStates(fmi3Instance instance, fmi3Float64 states[], size_t nx) {

    ASSERT_STATE(GetContinuousStates)

    if (invalidNumber(S, "fmi3GetContinuousStates", "nx", nx, NX))
        return fmi3Error;

    if (nullPointer(S, "fmi3GetContinuousStates", "states[]", states))
        return fmi3Error;

    getContinuousStates(S, states, nx);

    return fmi3OK;
}

fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Instance instance, fmi3Float64 x_nominal[], size_t nx) {

    ASSERT_STATE(GetNominalsOfContinuousStates)

    if (invalidNumber(S, "fmi3GetNominalContinuousStates", "nx", nx, NX))
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

    S->state = StepMode;

    return fmi3OK;
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
                      fmi3Boolean* eventEncountered,
                      fmi3Boolean* terminateSimulation,
                      fmi3Boolean* earlyReturn,
                      fmi3Float64* lastSuccessfulTime) {

    ASSERT_STATE(DoStep)

    if (communicationStepSize <= 0) {
        logError(S, "fmi3DoStep: communication step size must be > 0 but was %g.", communicationStepSize);
        S->state = modelError;
        return fmi3Error;
    }

    // TODO: pass to doStep()
    *terminateSimulation = fmi3False;

    int earlyReturn_;

    Status status = doStep(S, currentCommunicationPoint, currentCommunicationPoint + communicationStepSize, &earlyReturn_, lastSuccessfulTime);

    *earlyReturn = earlyReturn_;

    return (fmi3Status)status;
}

fmi3Status fmi3ActivateModelPartition(fmi3Instance instance,
                                      fmi3ValueReference clockReference,
                                      size_t clockElementIndex,
                                      fmi3Float64 activationTime) {

    ASSERT_STATE(ActivateModelPartition)

    return (fmi3Status)activateModelPartition(S, clockReference, activationTime);
}


/***************************************************
 Utility functions shared across FMUs
 ****************************************************/

ModelInstance* createModelInstance(
    loggerType cbLogger,
    intermediateUpdateType intermediateUpdate,
    void* componentEnvironment,
    const char* instanceName,
    const char* instantiationToken,
    const char* resourceLocation,
    bool loggingOn,
    InterfaceType interfaceType,
    bool returnEarly) {

    ModelInstance* comp = NULL;

    if (!cbLogger) {
        return NULL;
    }

    if (!instanceName || strlen(instanceName) == 0) {
        cbLogger(componentEnvironment, "?", Error, "error", "Missing instance name.");
        return NULL;
    }

    if (!instantiationToken || strlen(instantiationToken) == 0) {
        cbLogger(componentEnvironment, instanceName, Error, "error", "Missing GUID.");
        return NULL;
    }

    if (strcmp(instantiationToken, INSTANTIATION_TOKEN)) {
        cbLogger(componentEnvironment, instanceName, Error, "error", "Wrong GUID.");
        return NULL;
    }

    comp = (ModelInstance*)calloc(1, sizeof(ModelInstance));

    if (comp) {

        // set the callbacks
        comp->componentEnvironment = componentEnvironment;
        comp->logger = cbLogger;
        comp->intermediateUpdate = intermediateUpdate;
        comp->lockPreemtion = NULL;
        comp->unlockPreemtion = NULL;

        comp->instanceName = (char*)calloc(1 + strlen(instanceName), sizeof(char));

        // resourceLocation is NULL for FMI 1.0 ME
        if (resourceLocation) {
            comp->resourceLocation = (char*)calloc(1 + strlen(resourceLocation), sizeof(char));
            strcpy((char*)comp->resourceLocation, (char*)resourceLocation);
        }
        else {
            comp->resourceLocation = NULL;
        }

        comp->status = OK;

        comp->modelData = (ModelData*)calloc(1, sizeof(ModelData));

        comp->logEvents = loggingOn;
        comp->logErrors = true; // always log errors

        comp->nSteps = 0;

        comp->returnEarly = false;
    }

    if (!comp || !comp->modelData || !comp->instanceName) {
        logError(comp, "Out of memory.");
        return NULL;
    }

    comp->time = 0; // overwrite in fmi*SetupExperiment, fmi*SetTime
    strcpy((char*)comp->instanceName, (char*)instanceName);
    comp->type = interfaceType;

    comp->state = Instantiated;
    comp->isNewEventIteration = false;

    comp->newDiscreteStatesNeeded = false;
    comp->terminateSimulation = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->valuesOfContinuousStatesChanged = false;
    comp->nextEventTimeDefined = false;
    comp->nextEventTime = 0;

    setStartValues(comp); // to be implemented by the includer of this file
    comp->isDirtyValues = true; // because we just called setStartValues

#if NZ > 0
    comp->z = calloc(sizeof(double), NZ);
    comp->prez = calloc(sizeof(double), NZ);
#else
    comp->z = NULL;
    comp->prez = NULL;
#endif

    return comp;
}

void freeModelInstance(ModelInstance *comp) {
    free((void *)comp->instanceName);
    free(comp->z);
    free(comp->prez);
    free(comp);
}

bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, size_t actual, size_t expected) {

    if (actual != expected) {
        comp->state = modelError;
        logError(comp, "%s: Invalid argument %s = %d. Expected %d.", f, arg, actual, expected);
        return true;
    }

    return false;
}

bool invalidState(ModelInstance *comp, const char *f, int statesExpected) {

    if (!comp) {
        return true;
    }

    // TODO: add missing states and check state
    return false;

//    if (!(comp->state & statesExpected)) {
//        comp->state = modelError;
//        logError(comp, "%s: Illegal call sequence.", f);
//        return true;
//    }
//
//    return false;
}

bool nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {

    if (!p) {
        comp->state = modelError;
        logError(comp, "%s: Invalid argument %s = NULL.", f, arg);
        return true;
    }

    return false;
}

Status setDebugLogging(ModelInstance *comp, bool loggingOn, size_t nCategories, const char * const categories[]) {

    if (loggingOn) {
        for (size_t i = 0; i < nCategories; i++) {
            if (categories[i] == NULL) {
                logError(comp, "Log category[%d] must not be NULL", i);
                return Error;
            } else if (strcmp(categories[i], "logEvents") == 0) {
                comp->logEvents = true;
            } else if (strcmp(categories[i], "logStatusError") == 0) {
                comp->logErrors = true;
            } else {
                logError(comp, "Log category[%d] must be one of logEvents or logStatusError but was %s", i, categories[i]);
                return Error;
            }
        }
    } else {
        // disable logging
        comp->logEvents = false;
        comp->logErrors = false;
    }

    return OK;
}

static void logMessage(ModelInstance *comp, int status, const char *category, const char *message, va_list args) {

    va_list args1;
    size_t len = 0;
    char *buf = "";

    va_copy(args1, args);
    len = vsnprintf(buf, len, message, args1);
    va_end(args1);

    va_copy(args1, args);
    buf = (char *)calloc(len + 1, sizeof(char));
    vsnprintf(buf, len + 1, message, args);
    va_end(args1);

    // no need to distinguish between FMI versions since we're not using variadic arguments
    comp->logger(comp->componentEnvironment, comp->instanceName, status, category, buf);

    free(buf);
}

void logEvent(ModelInstance *comp, const char *message, ...) {

    if (!comp || !comp->logEvents) return;

    va_list args;
    va_start(args, message);
    logMessage(comp, OK, "logEvents", message, args);
    va_end(args);
}

void logError(ModelInstance *comp, const char *message, ...) {

    if (!comp || !comp->logErrors) return;

    va_list args;
    va_start(args, message);
    logMessage(comp, Error, "logStatusError", message, args);
    va_end(args);
}

// default implementations
#if NZ < 1
void getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    UNUSED(comp)
    UNUSED(z)
    UNUSED(nz)
    // do nothing
}
#endif

#ifndef GET_FLOAT64
Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_UINT16
Status getUInt16(ModelInstance* comp, ValueReference vr, uint16_t *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_INT32
Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_BOOLEAN
Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_STRING
Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef GET_BINARY
Status getBinary(ModelInstance* comp, ValueReference vr, size_t size[], const char *value[], size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(size)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_FLOAT64
Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_UINT16
Status setUInt16(ModelInstance* comp, ValueReference vr, const uint16_t *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_INT32
Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_BOOLEAN
Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_STRING
Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef SET_BINARY
Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char *const value[], size_t *index) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(size)
    UNUSED(value)
    UNUSED(index)
    return Error;
}
#endif

#ifndef ACTIVATE_CLOCK
Status activateClock(ModelInstance* comp, ValueReference vr) {
    UNUSED(comp)
    UNUSED(vr)
    return Error;
}
#endif

#ifndef GET_CLOCK
Status getClock(ModelInstance* comp, ValueReference vr, bool* value) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(value)
    return Error;
}
#endif

#ifndef GET_INTERVAL
Status getInterval(ModelInstance* comp, ValueReference vr, double* interval, int* qualifier) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(interval)
    UNUSED(qualifier)
    return Error;
}
#endif

#ifndef ACTIVATE_MODEL_PARTITION
Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime) {
    UNUSED(comp)
    UNUSED(vr)
    UNUSED(activationTime)
    return Error;
}
#endif

#if NX < 1
void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(comp)
    UNUSED(x)
    UNUSED(nx)
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(comp)
    UNUSED(x)
    UNUSED(nx)
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(comp)
    UNUSED(dx)
    UNUSED(nx)
}
#endif

#ifndef GET_PARTIAL_DERIVATIVE
Status getPartialDerivative(ModelInstance *comp, ValueReference unknown, ValueReference known, double *partialDerivative) {
    UNUSED(comp)
    UNUSED(unknown)
    UNUSED(known)
    UNUSED(partialDerivative)
    return Error;
}
#endif

Status doStep(ModelInstance *comp, double t, double tNext, int* earlyReturn, double* lastSuccessfulTime) {

    UNUSED(t)  // TODO: check t == comp->time ?

    bool stateEvent, timeEvent;
    Status status = OK;

#if NZ > 0
    double *temp = NULL;
#endif

#if NX > 0
    double  x[NX] = { 0 };
    double dx[NX] = { 0 };
#endif

    double epsilon = (1.0 + fabs(comp->time)) * DBL_EPSILON;

    while (comp->time + FIXED_SOLVER_STEP < tNext + epsilon) {

#if NX > 0
        getContinuousStates(comp, x, NX);
        getDerivatives(comp, dx, NX);

        // forward Euler step
        for (int i = 0; i < NX; i++) {
            x[i] += FIXED_SOLVER_STEP * dx[i];
        }

        setContinuousStates(comp, x, NX);
#endif

        stateEvent = false;

#if NZ > 0
        getEventIndicators(comp, comp->z, NZ);

        // check for zero-crossings
        for (int i = 0; i < NZ; i++) {
            stateEvent |= comp->prez[i] < 0 && comp->z[i] >= 0;
            stateEvent |= comp->prez[i] > 0 && comp->z[i] <= 0;
        }

        // remember the current event indicators
        temp = comp->z;
        comp->z = comp->prez;
        comp->prez = temp;
#endif

        // check for time event
        timeEvent = comp->nextEventTimeDefined && (comp->time + FIXED_SOLVER_STEP * 1e-2) >= comp->nextEventTime;

        // log events
        if (timeEvent) logEvent(comp, "Time event detected at t=%g s.", comp->time);
        if (stateEvent) logEvent(comp, "State event detected at t=%g s.", comp->time);

        if (stateEvent || timeEvent) {

            eventUpdate(comp);

            comp->returnEarly = comp->nextEventTime < t + tNext;

#if NZ > 0
            // update previous event indicators
            getEventIndicators(comp, comp->prez, NZ);
#endif

#if FMI_VERSION == 3
            if (comp->intermediateUpdate) {

                comp->state = IntermediateUpdateMode;

                bool earlyReturnRequested;
                double earlyReturnTime;

                comp->intermediateUpdate((fmi3InstanceEnvironment)comp->componentEnvironment,
                                          comp->time,         // intermediateUpdateTime
                                          comp->clocksTicked, // clocksTicked
                                          false,              // intermediateVariableSetRequested
                                          true,               // intermediateVariableGetAllowed
                                          false,              // intermediateStepFinished
                                          true,               // canReturnEarly
                                          &earlyReturnRequested,
                                          &earlyReturnTime);

                comp->state = StepMode;

                if (earlyReturnRequested) {
                    *earlyReturn = 1;
                    // TODO: continue to earlyReturnTime?
                    return status;
                }
            }
#endif
        }

        // terminate simulation, if requested by the model in the previous step
        if (comp->terminateSimulation) {
#if FMI_VERSION == 2
            comp->state = StepFailed;
#endif
            return Discard; // enforce termination of the simulation loop
        }

        comp->time = FIXED_SOLVER_STEP * (++comp->nSteps);

#if FMI_VERSION == 3
        if (comp->intermediateUpdate) {

            comp->state = IntermediateUpdateMode;

            bool earlyReturnRequested;
            double earlyReturnTime;

            // call intermediate update callback
            comp->intermediateUpdate((fmi3InstanceEnvironment)comp->componentEnvironment,
                                      comp->time, // intermediateUpdateTime
                                      false,      // clocksTicked
                                      false,      // intermediateVariableSetRequested
                                      true,       // intermediateVariableGetAllowed
                                      true,       // intermediateStepFinished
                                      true,       // canReturnEarly
                                      &earlyReturnRequested,
                                      &earlyReturnTime);

            comp->state = StepMode;
        }
#endif
    }

    if (earlyReturn) {
        *earlyReturn = 0;
    }

    if (lastSuccessfulTime) {
        *lastSuccessfulTime = comp->time;
    }

    return status;
}
