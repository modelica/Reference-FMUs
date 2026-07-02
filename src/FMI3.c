#ifdef _WIN32
#include <direct.h>
#else
#include <stdarg.h>
#include <dlfcn.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "FMI3.h"


static void cb_logMessage3(fmi3InstanceEnvironment instanceEnvironment,
                           fmi3Status status,
                           fmi3String category,
                           fmi3String message) {

    if (!instanceEnvironment) return;

    FMIInstance *instance = instanceEnvironment;

    if (!instance->logMessage) return;

    instance->logMessage(instance, (FMIStatus)status, category, message);
}

#if defined(FMI3_FUNCTION_PREFIX)
#define LOAD_SYMBOL(f) \
do { \
    instance->fmi3Functions->fmi3 ## f = fmi3 ## f; \
} while (0)
#elif defined(_WIN32)
#define LOAD_SYMBOL(f) \
do { \
    instance->fmi3Functions->fmi3 ## f = (fmi3 ## f ## TYPE*)GetProcAddress(instance->libraryHandle, "fmi3" #f); \
    if (!instance->fmi3Functions->fmi3 ## f) { \
        instance->logMessage(instance, FMIFatal, "fatal", "Symbol fmi3" #f " is missing in shared library."); \
        return FMIFatal; \
    } \
} while (0)
#else
#define LOAD_SYMBOL(f) \
do { \
    instance->fmi3Functions->fmi3 ## f = (fmi3 ## f ## TYPE*)dlsym(instance->libraryHandle, "fmi3" #f); \
    if (!instance->fmi3Functions->fmi3 ## f) { \
        instance->logMessage(instance, FMIFatal, "fatal", "Symbol fmi3" #f " is missing in shared library."); \
        return FMIFatal; \
    } \
} while (0)
#endif

#define CALL(f) \
do { \
    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3 ## f (instance->component); \
    if (instance->logFunctionCall) { \
        instance->logFunctionCall(instance, status, "fmi3" #f "()"); \
    } \
    instance->status = status > instance->status ? status : instance->status; \
    return status; \
} while (0)

#define CALL_ARGS(f, m, ...) \
do { \
    const FMIStatus status = (FMIStatus)instance->fmi3Functions-> fmi3 ## f (instance->component, __VA_ARGS__); \
    if (instance->logFunctionCall) { \
        FMIClearLogMessageBuffer(instance); \
        FMIAppendToLogMessageBuffer(instance, "fmi3" #f "(" m ")", __VA_ARGS__); \
        instance->logFunctionCall(instance, status, instance->logMessageBuffer); \
    } \
    instance->status = status > instance->status ? status : instance->status; \
    return status; \
} while (0)

#define CALL_ARRAY(s, t) \
do { \
    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3 ## s ## t(instance->component, valueReferences, nValueReferences, values, nValues); \
    if (instance->logFunctionCall) { \
        FMIClearLogMessageBuffer(instance); \
        FMIAppendToLogMessageBuffer(instance, "fmi3" #s #t "(valueReferences={"); \
        FMIAppendArrayToLogMessageBuffer(instance, valueReferences, nValueReferences, NULL, FMIValueReferenceType); \
        FMIAppendToLogMessageBuffer(instance, "}, nValueReferences=%zu, values={", nValueReferences); \
        FMIAppendArrayToLogMessageBuffer(instance, values, nValues, NULL, FMI ## t ## Type); \
        FMIAppendToLogMessageBuffer(instance, "}, nValues=%zu)", nValues); \
        instance->logFunctionCall(instance, status, instance->logMessageBuffer); \
    } \
    instance->status = status > instance->status ? status : instance->status; \
    return status; \
} while (0)

/***************************************************
Types for Common Functions
****************************************************/

/* Inquire version numbers and setting logging status */
const char* FMI3GetVersion(FMIInstance *instance) {
    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmi3GetVersion()");
    }
    return instance->fmi3Functions->fmi3GetVersion();
}

FMIStatus FMI3SetDebugLogging(FMIInstance *instance,
    fmi3Boolean loggingOn,
    size_t nCategories,
    const fmi3String categories[]) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3SetDebugLogging(instance->component, loggingOn, nCategories, categories);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3SetDebugLogging(loggingOn=%d, nCategories=%zu, categories={");
        FMIAppendArrayToLogMessageBuffer(instance, categories, nCategories, NULL, FMIStringType);
        FMIAppendToLogMessageBuffer(instance, "})");
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

static FMIStatus loadSymbols3(FMIInstance *instance) {

#if !defined(FMI_VERSION) || FMI_VERSION == 3

    instance->fmi3Functions = calloc(1, sizeof(FMI3Functions));

    if (!instance->fmi3Functions) {
        return FMIError;
    }

    instance->fmiMajorVersion = FMIMajorVersion3;

    /***************************************************
    Common Functions
    ****************************************************/

    /* Inquire version numbers and set debug logging */
    LOAD_SYMBOL(GetVersion);
    LOAD_SYMBOL(SetDebugLogging);

    /* Creation and destruction of FMU instances */
    LOAD_SYMBOL(InstantiateModelExchange);
    LOAD_SYMBOL(InstantiateCoSimulation);
    LOAD_SYMBOL(InstantiateScheduledExecution);
    LOAD_SYMBOL(FreeInstance);

    /* Enter and exit initialization mode, terminate and reset */
    LOAD_SYMBOL(EnterInitializationMode);
    LOAD_SYMBOL(ExitInitializationMode);
    LOAD_SYMBOL(EnterEventMode);
    LOAD_SYMBOL(Terminate);
    LOAD_SYMBOL(Reset);

    /* Getting and setting variable values */
    LOAD_SYMBOL(GetFloat32);
    LOAD_SYMBOL(GetFloat64);
    LOAD_SYMBOL(GetInt8);
    LOAD_SYMBOL(GetUInt8);
    LOAD_SYMBOL(GetInt16);
    LOAD_SYMBOL(GetUInt16);
    LOAD_SYMBOL(GetInt32);
    LOAD_SYMBOL(GetUInt32);
    LOAD_SYMBOL(GetInt64);
    LOAD_SYMBOL(GetUInt64);
    LOAD_SYMBOL(GetBoolean);
    LOAD_SYMBOL(GetString);
    LOAD_SYMBOL(GetBinary);
    LOAD_SYMBOL(GetClock);
    LOAD_SYMBOL(SetFloat32);
    LOAD_SYMBOL(SetFloat64);
    LOAD_SYMBOL(SetInt8);
    LOAD_SYMBOL(SetUInt8);
    LOAD_SYMBOL(SetInt16);
    LOAD_SYMBOL(SetUInt16);
    LOAD_SYMBOL(SetInt32);
    LOAD_SYMBOL(SetUInt32);
    LOAD_SYMBOL(SetInt64);
    LOAD_SYMBOL(SetUInt64);
    LOAD_SYMBOL(SetBoolean);
    LOAD_SYMBOL(SetString);
    LOAD_SYMBOL(SetBinary);
    LOAD_SYMBOL(SetClock);

    /* Getting Variable Dependency Information */
    LOAD_SYMBOL(GetNumberOfVariableDependencies);
    LOAD_SYMBOL(GetVariableDependencies);

    /* Getting and setting the internal FMU state */
    LOAD_SYMBOL(GetFMUState);
    LOAD_SYMBOL(SetFMUState);
    LOAD_SYMBOL(FreeFMUState);
    LOAD_SYMBOL(SerializedFMUStateSize);
    LOAD_SYMBOL(SerializeFMUState);
    LOAD_SYMBOL(DeserializeFMUState);

    /* Getting partial derivatives */
    LOAD_SYMBOL(GetDirectionalDerivative);
    LOAD_SYMBOL(GetAdjointDerivative);

    /* Entering and exiting the Configuration or Reconfiguration Mode */
    LOAD_SYMBOL(EnterConfigurationMode);
    LOAD_SYMBOL(ExitConfigurationMode);

    /* Clock related functions */
    LOAD_SYMBOL(GetIntervalDecimal);
    LOAD_SYMBOL(GetIntervalFraction);
    LOAD_SYMBOL(GetShiftDecimal);
    LOAD_SYMBOL(GetShiftFraction);
    LOAD_SYMBOL(SetIntervalDecimal);
    LOAD_SYMBOL(SetIntervalFraction);
    LOAD_SYMBOL(EvaluateDiscreteStates);
    LOAD_SYMBOL(UpdateDiscreteStates);

    /***************************************************
    Functions for Model Exchange
    ****************************************************/

    LOAD_SYMBOL(EnterContinuousTimeMode);
    LOAD_SYMBOL(CompletedIntegratorStep);

    /* Providing independent variables and re-initialization of caching */
    LOAD_SYMBOL(SetTime);
    LOAD_SYMBOL(SetContinuousStates);

    /* Evaluation of the model equations */
    LOAD_SYMBOL(GetContinuousStateDerivatives);
    LOAD_SYMBOL(GetEventIndicators);
    LOAD_SYMBOL(GetContinuousStates);
    LOAD_SYMBOL(GetNominalsOfContinuousStates);
    LOAD_SYMBOL(GetNumberOfEventIndicators);
    LOAD_SYMBOL(GetNumberOfContinuousStates);

    /***************************************************
    Functions for Co-Simulation
    ****************************************************/

    /* Simulating the FMU */
    LOAD_SYMBOL(EnterStepMode);
    LOAD_SYMBOL(GetOutputDerivatives);
    LOAD_SYMBOL(DoStep);
    LOAD_SYMBOL(ActivateModelPartition);

    instance->state = FMIStartAndEndState;

    return FMIOK;

#else

    return FMIError;

#endif
}

/* Creation and destruction of FMU instances and setting debug status */
FMIStatus FMI3InstantiateModelExchange(
    FMIInstance *instance,
    fmi3String   instantiationToken,
    fmi3String   resourcePath,
    fmi3Boolean  visible,
    fmi3Boolean  loggingOn) {

    const FMIStatus status = loadSymbols3(instance);

    fmi3LogMessageCallback logMessage = instance->logMessage ? cb_logMessage3 : NULL;

    instance->component = instance->fmi3Functions->fmi3InstantiateModelExchange(instance->name, instantiationToken, resourcePath, visible, loggingOn, instance, logMessage);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi3InstantiateModelExchange("
            "instanceName=\"%s\", "
            "instantiationToken=\"%s\", "
            "resourcePath=\"%s\", "
            "visible=%d, "
            "loggingOn=%d, "
            "instanceEnvironment=0x%p, "
            "logMessage=0x%p)",
            instance->name,
            instantiationToken,
            resourcePath,
            visible,
            loggingOn,
            instance,
            logMessage);
        instance->logFunctionCall(instance, instance->component ? FMIOK : FMIError, instance->logMessageBuffer);
    }

    if (!instance->component) {
        return FMIError;
    }

    instance->interfaceType = FMIModelExchange;
    instance->state = FMIInstantiatedState;

    return status;
}

FMIStatus FMI3InstantiateCoSimulation(
    FMIInstance                   *instance,
    fmi3String                     instantiationToken,
    fmi3String                     resourcePath,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3Boolean                    eventModeUsed,
    fmi3Boolean                    earlyReturnAllowed,
    const fmi3ValueReference       requiredIntermediateVariables[],
    size_t                         nRequiredIntermediateVariables,
    fmi3IntermediateUpdateCallback intermediateUpdate) {

    if (loadSymbols3(instance) != FMIOK) {
        return FMIFatal;
    }

    fmi3LogMessageCallback logMessage = instance->logMessage ? cb_logMessage3 : NULL;

    instance->component = instance->fmi3Functions->fmi3InstantiateCoSimulation(
        instance->name,
        instantiationToken,
        resourcePath,
        visible,
        loggingOn,
        eventModeUsed,
        earlyReturnAllowed,
        requiredIntermediateVariables,
        nRequiredIntermediateVariables,
        instance,
        logMessage,
        intermediateUpdate);

    instance->fmi3Functions->eventModeUsed = eventModeUsed;

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi3InstantiateCoSimulation("
            "instanceName=\"%s\", "
            "instantiationToken=\"%s\", "
            "resourcePath=\"%s\", "
            "visible=%d, "
            "loggingOn=%d, "
            "eventModeUsed=%d, "
            "earlyReturnAllowed=%d, "
            "requiredIntermediateVariables=0x%p, "
            "nRequiredIntermediateVariables=%zu, "
            "instanceEnvironment=0x%p, "
            "logMessage=0x%p, "
            "intermediateUpdate=0x%p)",
            instance->name,
            instantiationToken,
            resourcePath,
            visible,
            loggingOn,
            eventModeUsed,
            earlyReturnAllowed,
            requiredIntermediateVariables,
            nRequiredIntermediateVariables,
            instance,
            logMessage,
            intermediateUpdate);
        instance->logFunctionCall(instance, instance->component ? FMIOK : FMIError, instance->logMessageBuffer);
    }

    if (!instance->component) {
        return FMIError;
    }

    instance->interfaceType = FMICoSimulation;
    instance->state = FMIInstantiatedState;

    return FMIOK;
}

FMIStatus FMI3InstantiateScheduledExecution(
    FMIInstance                   *instance,
    fmi3String                     instantiationToken,
    fmi3String                     resourcePath,
    fmi3Boolean                    visible,
    fmi3Boolean                    loggingOn,
    fmi3InstanceEnvironment        instanceEnvironment,
    fmi3LogMessageCallback         logMessage,
    fmi3ClockUpdateCallback        clockUpdate,
    fmi3LockPreemptionCallback     lockPreemption,
    fmi3UnlockPreemptionCallback   unlockPreemption) {

    if (loadSymbols3(instance) != FMIOK) {
        return FMIError;
    }

    fmi3LogMessageCallback _logMessage = instance->logMessage ? cb_logMessage3 : NULL;

    instance->component = instance->fmi3Functions->fmi3InstantiateScheduledExecution(
        instance->name,
        instantiationToken,
        resourcePath,
        visible,
        loggingOn,
        instance,
        _logMessage,
        clockUpdate,
        lockPreemption,
        unlockPreemption);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi3InstantiateScheduledExecution("
            "instanceName=\"%s\", "
            "instantiationToken=\"%s\", "
            "resourcePath=\"%s\", "
            "visible=%d, "
            "loggingOn=%d, "
            "instanceEnvironment=0x%p, "
            "logMessage=0x%p, "
            "clockUpdate=0x%p, "
            "lockPreemption=0x%p, "
            "unlockPreemption=0x%p)",
            instance->name,
            instantiationToken,
            resourcePath,
            visible,
            loggingOn,
            instance,
            _logMessage,
            clockUpdate,
            lockPreemption,
            unlockPreemption);
        instance->logFunctionCall(instance, instance->component ? FMIOK : FMIError, instance->logMessageBuffer);
    }

    if (!instance->component) {
        return FMIError;
    }

    instance->interfaceType = FMIScheduledExecution;
    instance->state = FMIInstantiatedState;

    return FMIOK;
}

FMIStatus FMI3FreeInstance(FMIInstance *instance) {

    if (!instance) {
        return FMIError;
    }

    instance->fmi3Functions->fmi3FreeInstance(instance->component);

    instance->component = NULL;

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmi3FreeInstance()");
    }

    return FMIOK;
}

/* Enter and exit initialization mode, enter event mode, terminate and reset */
FMIStatus FMI3EnterInitializationMode(FMIInstance *instance,
    fmi3Boolean toleranceDefined,
    fmi3Float64 tolerance,
    fmi3Float64 startTime,
    fmi3Boolean stopTimeDefined,
    fmi3Float64 stopTime) {

    instance->state = FMIInitializationModeState;

    CALL_ARGS(EnterInitializationMode,
        "toleranceDefined=%d, tolerance=%.16g, startTime=%.16g, stopTimeDefined=%d, stopTime=%.16g",
        toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
}

FMIStatus FMI3ExitInitializationMode(FMIInstance *instance) {

    if (instance->interfaceType == FMIModelExchange) {
        instance->state = FMIEventModeState;
    } else if (instance->interfaceType == FMICoSimulation) {
        instance->state = instance->fmi3Functions->eventModeUsed ? FMIEventModeState : FMIStepModeState;
    } else {
        instance->state = FMIClockActivationMode;
    }

    CALL(ExitInitializationMode);
}

FMIStatus FMI3EnterEventMode(FMIInstance *instance) {
    instance->state = FMIEventModeState;
    CALL(EnterEventMode);
}

FMIStatus FMI3Terminate(FMIInstance *instance) {
    instance->state = FMITerminatedState;
    CALL(Terminate);
}

FMIStatus FMI3Reset(FMIInstance *instance) {
    instance->state = FMIInstantiatedState;
    CALL(Reset);
}

/* Getting and setting variable values */
FMIStatus FMI3GetFloat32(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float32 values[],
    size_t nValues) {
    CALL_ARRAY(Get, Float32);
}

FMIStatus FMI3GetFloat64(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 values[],
    size_t nValues) {
    CALL_ARRAY(Get, Float64);
}

FMIStatus FMI3GetInt8(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int8 values[],
    size_t nValues) {
    CALL_ARRAY(Get, Int8);
}

FMIStatus FMI3GetUInt8(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt8 values[],
    size_t nValues) {
    CALL_ARRAY(Get, UInt8);
}

FMIStatus FMI3GetInt16(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int16 values[],
    size_t nValues) {
    CALL_ARRAY(Get, Int16);
}

FMIStatus FMI3GetUInt16(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt16 values[],
    size_t nValues) {
    CALL_ARRAY(Get, UInt16);
}

FMIStatus FMI3GetInt32(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int32 values[],
    size_t nValues) {
    CALL_ARRAY(Get, Int32);
}

FMIStatus FMI3GetUInt32(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt32 values[],
    size_t nValues) {
    CALL_ARRAY(Get, UInt32);
}

FMIStatus FMI3GetInt64(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Int64 values[],
    size_t nValues) {
    CALL_ARRAY(Get, Int64);
}

FMIStatus FMI3GetUInt64(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt64 values[],
    size_t nValues) {
    CALL_ARRAY(Get, UInt64);
}

FMIStatus FMI3GetBoolean(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Boolean values[],
    size_t nValues) {
    CALL_ARRAY(Get, Boolean);
}

FMIStatus FMI3GetString(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3String values[],
    size_t nValues) {
    CALL_ARRAY(Get, String);
}

FMIStatus FMI3GetBinary(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    size_t sizes[],
    fmi3Binary values[],
    size_t nValues) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3GetBinary(instance->component, valueReferences, nValueReferences, sizes, values, nValues);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3GetBinary(valueReferences={");
        FMIAppendArrayToLogMessageBuffer(instance, valueReferences, nValueReferences, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nValueReferences=%zu, sizes={", nValueReferences);
        FMIAppendArrayToLogMessageBuffer(instance, sizes, nValueReferences, NULL, FMISizeTType);
        FMIAppendToLogMessageBuffer(instance, "}, values={");
        FMIAppendArrayToLogMessageBuffer(instance, values, nValues, sizes, FMIBinaryType);
        FMIAppendToLogMessageBuffer(instance, "}, nValues=%zu)", nValues);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3GetClock(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Clock values[]) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3GetClock(instance->component, valueReferences, nValueReferences, values);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3GetClock(valueReferences={");
        FMIAppendArrayToLogMessageBuffer(instance, valueReferences, nValueReferences, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nValueReferences=%zu, values={", nValueReferences);
        FMIAppendArrayToLogMessageBuffer(instance, values, nValueReferences, NULL, FMIClockType);
        FMIAppendToLogMessageBuffer(instance, "})");
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3SetFloat32(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float32 values[],
    size_t nValues)    {
    CALL_ARRAY(Set, Float32);
}

FMIStatus FMI3SetFloat64(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float64 values[],
    size_t nValues) {
    CALL_ARRAY(Set, Float64);
}

FMIStatus FMI3SetInt8(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int8 values[],
    size_t nValues) {
    CALL_ARRAY(Set, Int8);
}

FMIStatus FMI3SetUInt8(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt8 values[],
    size_t nValues) {
    CALL_ARRAY(Set, UInt8);
}

FMIStatus FMI3SetInt16(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int16 values[],
    size_t nValues) {
    CALL_ARRAY(Set, Int16);
}

FMIStatus FMI3SetUInt16(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt16 values[],
    size_t nValues) {
    CALL_ARRAY(Set, UInt16);
}

FMIStatus FMI3SetInt32(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int32 values[],
    size_t nValues) {
    CALL_ARRAY(Set, Int32);
}

FMIStatus FMI3SetUInt32(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt32 values[],
    size_t nValues) {
    CALL_ARRAY(Set, UInt32);
}

FMIStatus FMI3SetInt64(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int64 values[],
    size_t nValues) {
    CALL_ARRAY(Set, Int64);
}

FMIStatus FMI3SetUInt64(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt64 values[],
    size_t nValues) {
    CALL_ARRAY(Set, UInt64);
}

FMIStatus FMI3SetBoolean(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Boolean values[],
    size_t nValues) {
    CALL_ARRAY(Set, Boolean);
}

FMIStatus FMI3SetString(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3String values[],
    size_t nValues) {
    CALL_ARRAY(Set, String);
}

FMIStatus FMI3SetBinary(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const size_t sizes[],
    const fmi3Binary values[],
    size_t nValues) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3SetBinary(instance->component, valueReferences, nValueReferences, sizes, values, nValues);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3SetBinary(valueReferences={");
        FMIAppendArrayToLogMessageBuffer(instance, valueReferences, nValueReferences, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nValueReferences=%zu, sizes={", nValueReferences);
        FMIAppendArrayToLogMessageBuffer(instance, sizes, nValueReferences, NULL, FMISizeTType);
        FMIAppendToLogMessageBuffer(instance, "}, values={");
        FMIAppendArrayToLogMessageBuffer(instance, values, nValues, sizes, FMIBinaryType);
        FMIAppendToLogMessageBuffer(instance, "}, nValues=%zu)", nValues);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3SetClock(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Clock values[]) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3SetClock(instance->component, valueReferences, nValueReferences, values);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3SetClock(valueReferences={");
        FMIAppendArrayToLogMessageBuffer(instance, valueReferences, nValueReferences, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nValueReferences=%zu, values={", nValueReferences);
        FMIAppendArrayToLogMessageBuffer(instance, values, nValueReferences, NULL, FMIClockType);
        FMIAppendToLogMessageBuffer(instance, "})");
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/* Getting Variable Dependency Information */
FMIStatus FMI3GetNumberOfVariableDependencies(FMIInstance *instance,
    fmi3ValueReference valueReference,
    size_t* nDependencies) {
    CALL_ARGS(GetNumberOfVariableDependencies, "valueReference=%u, nDependencies=0x%p", valueReference, nDependencies);
}

FMIStatus FMI3GetVariableDependencies(FMIInstance *instance,
    fmi3ValueReference dependent,
    size_t elementIndicesOfDependent[],
    fmi3ValueReference independents[],
    size_t elementIndicesOfIndependents[],
    fmi3DependencyKind dependencyKinds[],
    size_t nDependencies) {
    CALL_ARGS(GetVariableDependencies, "dependent=%u, elementIndicesOfDependent=0x%p, independents=0x%p, elementIndicesOfIndependents=0x%p, dependencyKinds=0x%p, nDependencies=%zu",
        dependent, elementIndicesOfDependent, independents, elementIndicesOfIndependents, dependencyKinds, nDependencies);
}

/* Getting and setting the internal FMU state */
FMIStatus FMI3GetFMUState(FMIInstance *instance, fmi3FMUState* FMUState) {
    CALL_ARGS(GetFMUState, "FMUState=0x%p", FMUState);
}

FMIStatus FMI3SetFMUState(FMIInstance *instance, fmi3FMUState  FMUState) {
    CALL_ARGS(SetFMUState, "FMUState=0x%p", FMUState);
}

FMIStatus FMI3FreeFMUState(FMIInstance *instance, fmi3FMUState* FMUState) {
    CALL_ARGS(FreeFMUState, "FMUState=0x%p", FMUState);
}


FMIStatus FMI3SerializedFMUStateSize(FMIInstance *instance,
    fmi3FMUState  FMUState,
    size_t* size) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3SerializedFMUStateSize(instance->component, FMUState, size);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3SerializedFMUStateSize(FMUState=0x%p, size=%zu)", FMUState, *size);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3SerializeFMUState(FMIInstance *instance,
    fmi3FMUState  FMUState,
    fmi3Byte serializedState[],
    size_t size) {
    CALL_ARGS(SerializeFMUState, "FMUstate=0x%p, serializedState=0x%p, size=%zu", FMUState, serializedState, size);
}

FMIStatus FMI3DeserializeFMUState(FMIInstance *instance,
    const fmi3Byte serializedState[],
    size_t size,
    fmi3FMUState* FMUState) {
    CALL_ARGS(DeserializeFMUState, "serializedState=0x%p, size=%zu, FMUState=0x%p", serializedState, size, FMUState);
}

/* Getting partial derivatives */
FMIStatus FMI3GetDirectionalDerivative(FMIInstance *instance,
    const fmi3ValueReference unknowns[],
    size_t nUnknowns,
    const fmi3ValueReference knowns[],
    size_t nKnowns,
    const fmi3Float64 seed[],
    size_t nSeed,
    fmi3Float64 sensitivity[],
    size_t nSensitivity) {
    CALL_ARGS(GetDirectionalDerivative,
        "unknowns=0x%p, nUnknowns=%zu, knowns=0x%p, nKnowns=%zu, seed=0x%p, nSeed=%zu, sensitivity=0x%p, nSensitivity=%zu",
        unknowns, nUnknowns, knowns, nKnowns, seed, nSeed, sensitivity, nSensitivity);
}

FMIStatus FMI3GetAdjointDerivative(FMIInstance *instance,
    const fmi3ValueReference unknowns[],
    size_t nUnknowns,
    const fmi3ValueReference knowns[],
    size_t nKnowns,
    const fmi3Float64 seed[],
    size_t nSeed,
    fmi3Float64 sensitivity[],
    size_t nSensitivity) {
    CALL_ARGS(GetAdjointDerivative,
        "unknowns=0x%p, nUnknowns=%zu, knowns=0x%p, nKnowns=%zu, seed=0x%p, nSeed=%zu, sensitivity=0x%p, nSensitivity=%zu",
        unknowns, nUnknowns, knowns, nKnowns, seed, nSeed, sensitivity, nSensitivity);
}

/* Entering and exiting the Configuration or Reconfiguration Mode */
FMIStatus FMI3EnterConfigurationMode(FMIInstance *instance) {
    instance->state = instance->state == FMIInstantiatedState ? FMIConfigurationModeState: FMIReconfigurationModeState;
    CALL(EnterConfigurationMode);
}

FMIStatus FMI3ExitConfigurationMode(FMIInstance *instance) {

    if (instance->state == FMIConfigurationModeState) {

        instance->state = FMIInstantiatedState;

    } else if (instance->state == FMIReconfigurationModeState) {

        if (instance->interfaceType == FMIModelExchange) {
            instance->state = FMIEventModeState;
        } else if (instance->interfaceType == FMICoSimulation) {
            instance->state = FMIStepModeState;
        } else {
            instance->state = FMIClockActivationMode;
        }

    } else {

        return FMIError;

    }

    CALL(ExitConfigurationMode);
}

/* Clock related functions */

FMI_STATIC FMIStatus FMI3GetIntervalDecimal(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 intervals[],
    fmi3IntervalQualifier qualifiers[]) {
    CALL_ARGS(GetIntervalDecimal,
        "valueReferences=0x%p, nValueReferences=%zu, intervals=0x%p, qualifiers=0x%p",
        valueReferences, nValueReferences, intervals, qualifiers);
}

FMIStatus FMI3GetIntervalFraction(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt64 intervalCounters[],
    fmi3UInt64 resolutions[],
    fmi3IntervalQualifier qualifiers[]) {
    CALL_ARGS(GetIntervalFraction,
        "valueReferences=0x%p, nValueReferences=%zu, intervalCounters=0x%p, resolutions=0x%p, qualifiers=%d",
        valueReferences, nValueReferences, intervalCounters, resolutions, qualifiers);
}

FMIStatus FMI3GetShiftDecimal(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3Float64 shifts[]) {
    CALL_ARGS(GetShiftDecimal,
        "valueReferences=0x%p, nValueReferences=%zu, shifts=0x%p",
        valueReferences, nValueReferences, shifts);
}

FMIStatus FMI3GetShiftFraction(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    fmi3UInt64 shiftCounters[],
    fmi3UInt64 resolutions[]) {
    CALL_ARGS(GetShiftFraction,
        "valueReferences=0x%p, nValueReferences=%zu, shiftCounters=0x%p, resolutions=0x%p",
        valueReferences, nValueReferences, shiftCounters, resolutions);
}

FMIStatus FMI3SetIntervalDecimal(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float64 intervals[]) {
    CALL_ARGS(SetIntervalDecimal,
        "valueReferences=0x%p, nValueReferences=%zu, intervals=0x%p",
        valueReferences, nValueReferences, intervals);
}

FMIStatus FMI3SetIntervalFraction(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt64 intervalCounters[],
    const fmi3UInt64 resolutions[]) {
    CALL_ARGS(SetIntervalFraction,
        "valueReferences=0x%p, nValueReferences=%zu, intervalCounters=0x%p, resolutions=0x%p",
        valueReferences, nValueReferences, intervalCounters, resolutions);
}

FMIStatus FMI3SetShiftDecimal(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Float64 shifts[]) {
    CALL_ARGS(SetShiftDecimal,
        "valueReferences=0x%p, nValueReferences=%zu, shifts=0x%p",
        valueReferences, nValueReferences, shifts);
}

FMIStatus FMI3SetShiftFraction(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3UInt64 shiftCounters[],
    const fmi3UInt64 resolutions[]) {
    CALL_ARGS(SetShiftFraction,
        "valueReferences=0x%p, nValueReferences=%zu, shiftCounters=0x%p, resolutions=0x%p",
        valueReferences, nValueReferences, shiftCounters, resolutions);
}

FMIStatus FMI3EvaluateDiscreteStates(FMIInstance *instance) {
    CALL(EvaluateDiscreteStates);
}

FMIStatus FMI3UpdateDiscreteStates(FMIInstance *instance,
    fmi3Boolean* discreteStatesNeedUpdate,
    fmi3Boolean* terminateSimulation,
    fmi3Boolean* nominalsOfContinuousStatesChanged,
    fmi3Boolean* valuesOfContinuousStatesChanged,
    fmi3Boolean* nextEventTimeDefined,
    fmi3Float64* nextEventTime) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3UpdateDiscreteStates(instance->component, discreteStatesNeedUpdate, terminateSimulation, nominalsOfContinuousStatesChanged, valuesOfContinuousStatesChanged, nextEventTimeDefined, nextEventTime);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi3UpdateDiscreteStates(discreteStatesNeedUpdate=%d, terminateSimulation=%d, nominalsOfContinuousStatesChanged=%d, valuesOfContinuousStatesChanged=%d, nextEventTimeDefined=%d, nextEventTime=%.16g)",
            *discreteStatesNeedUpdate, *terminateSimulation, *nominalsOfContinuousStatesChanged, *valuesOfContinuousStatesChanged, *nextEventTimeDefined, *nextEventTime);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/***************************************************
Types for Functions for Model Exchange
****************************************************/

FMIStatus FMI3EnterContinuousTimeMode(FMIInstance *instance) {
    instance->state = FMIContinuousTimeModeState;
    CALL(EnterContinuousTimeMode);
}

FMIStatus FMI3CompletedIntegratorStep(FMIInstance *instance,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* enterEventMode,
    fmi3Boolean* terminateSimulation) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3CompletedIntegratorStep(instance->component, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi3CompletedIntegratorStep(noSetFMUStatePriorToCurrentPoint=%d, enterEventMode=%d, terminateSimulation=%d)",
            noSetFMUStatePriorToCurrentPoint, *enterEventMode, *terminateSimulation);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/* Providing independent variables and re-initialization of caching */
FMIStatus FMI3SetTime(FMIInstance *instance, fmi3Float64 time) {
    CALL_ARGS(SetTime, "time=%.16g", time);
}

FMIStatus FMI3SetContinuousStates(FMIInstance *instance,
    const fmi3Float64 continuousStates[],
    size_t nContinuousStates) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3SetContinuousStates(instance->component, continuousStates, nContinuousStates);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3SetContinuousStates(continuousStates={");
        FMIAppendArrayToLogMessageBuffer(instance, continuousStates, nContinuousStates, NULL, FMIFloat64Type);
        FMIAppendToLogMessageBuffer(instance, "}, nContinuousStates=%zu)", nContinuousStates);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/* Evaluation of the model equations */
FMIStatus FMI3GetContinuousStateDerivatives(FMIInstance *instance,
    fmi3Float64 derivatives[],
    size_t nContinuousStates) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3GetContinuousStateDerivatives(instance->component, derivatives, nContinuousStates);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3GetContinuousStateDerivatives(derivatives={");
        FMIAppendArrayToLogMessageBuffer(instance, derivatives, nContinuousStates, NULL, FMIFloat64Type);
        FMIAppendToLogMessageBuffer(instance, "}, nContinuousStates=%zu)", nContinuousStates);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3GetEventIndicators(FMIInstance *instance,
    fmi3Float64 eventIndicators[],
    size_t nEventIndicators) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3GetEventIndicators(instance->component, eventIndicators, nEventIndicators);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3GetEventIndicators(eventIndicators={");
        FMIAppendArrayToLogMessageBuffer(instance, eventIndicators, nEventIndicators, NULL, FMIFloat64Type);
        FMIAppendToLogMessageBuffer(instance, "}, nEventIndicators=%zu)", nEventIndicators);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3GetContinuousStates(FMIInstance *instance,
    fmi3Float64 continuousStates[],
    size_t nContinuousStates) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3GetContinuousStates(instance->component, continuousStates, nContinuousStates);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3GetContinuousStates(continuousStates={");
        FMIAppendArrayToLogMessageBuffer(instance, continuousStates, nContinuousStates, NULL, FMIFloat64Type);
        FMIAppendToLogMessageBuffer(instance, "}, nContinuousStates=%zu)", nContinuousStates);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3GetNominalsOfContinuousStates(FMIInstance *instance,
    fmi3Float64 nominals[],
    size_t nContinuousStates) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3GetNominalsOfContinuousStates(instance->component, nominals, nContinuousStates);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi3GetNominalsOfContinuousStates(nominals={");
        FMIAppendArrayToLogMessageBuffer(instance, nominals, nContinuousStates, NULL, FMIFloat64Type);
        FMIAppendToLogMessageBuffer(instance, "}, nContinuousStates=%zu)", nContinuousStates);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}


FMIStatus FMI3GetNumberOfEventIndicators(FMIInstance *instance,
    size_t* nEventIndicators) {
    CALL_ARGS(GetNumberOfEventIndicators, "nEventIndicators=%zu", nEventIndicators);
}

FMIStatus FMI3GetNumberOfContinuousStates(FMIInstance *instance,
    size_t* nContinuousStates) {
    CALL_ARGS(GetNumberOfContinuousStates, "nContinuousStates=%zu", nContinuousStates);
}

/***************************************************
Types for Functions for Co-Simulation
****************************************************/

/* Simulating the FMU */

FMIStatus FMI3EnterStepMode(FMIInstance *instance) {
    CALL(EnterStepMode);
}

FMIStatus FMI3GetOutputDerivatives(FMIInstance *instance,
    const fmi3ValueReference valueReferences[],
    size_t nValueReferences,
    const fmi3Int32 orders[],
    fmi3Float64 values[],
    size_t nValues) {
    CALL_ARGS(GetOutputDerivatives,
        "valueReferences=0x%p, nValueReferences=%zu, orders=0x%p, values=0x%p, nValues=%zu",
        valueReferences, nValueReferences, orders, values, nValues);
}

FMIStatus FMI3DoStep(FMIInstance *instance,
    fmi3Float64 currentCommunicationPoint,
    fmi3Float64 communicationStepSize,
    fmi3Boolean noSetFMUStatePriorToCurrentPoint,
    fmi3Boolean* eventHandlingNeeded,
    fmi3Boolean* terminateSimulation,
    fmi3Boolean* earlyReturn,
    fmi3Float64* lastSuccessfulTime) {

    const FMIStatus status = (FMIStatus)instance->fmi3Functions->fmi3DoStep(instance->component, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint, eventHandlingNeeded, terminateSimulation, earlyReturn, lastSuccessfulTime);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi3DoStep(currentCommunicationPoint=%.16g, communicationStepSize=%.16g, noSetFMUStatePriorToCurrentPoint=%d, eventHandlingNeeded=%d, terminateSimulation=%d, earlyReturn=%d, lastSuccessfulTime=%.16g)",
            currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint, *eventHandlingNeeded, *terminateSimulation, *earlyReturn, *lastSuccessfulTime);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI3ActivateModelPartition(FMIInstance *instance,
    fmi3ValueReference clockReference,
    fmi3Float64 activationTime) {
    CALL_ARGS(ActivateModelPartition,
        "clockReference=%u, activationTime=%.16g",
        clockReference, activationTime);
}

#undef LOAD_SYMBOL
#undef CALL
#undef CALL_ARGS
#undef CALL_ARRAY
