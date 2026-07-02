#ifdef _WIN32
#include <direct.h>
#define strdup _strdup
#define INTERNET_MAX_URL_LENGTH 2083 // from wininet.h
#else
#include <stdarg.h>
#include <dlfcn.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "FMI2.h"


static void cb_logMessage2(fmi2ComponentEnvironment componentEnvironment, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...) {

    if (!componentEnvironment) return;

    FMIInstance *instance = componentEnvironment;

    char buf[FMI_MAX_MESSAGE_LENGTH];

    va_list args;

    va_start(args, message);
    vsnprintf(buf, FMI_MAX_MESSAGE_LENGTH, message, args);
    va_end(args);

    if (!instance->logMessage) return;

    instance->logMessage(instance, (FMIStatus)status, category, buf);
}

#if defined(FMI2_FUNCTION_PREFIX)
#define LOAD_SYMBOL(f) \
do { \
    instance->fmi2Functions->fmi2 ## f = fmi2 ## f; \
} while (0)
#elif defined(_WIN32)
#define LOAD_SYMBOL(f) \
do { \
    instance->fmi2Functions->fmi2 ## f = (fmi2 ## f ## TYPE*)GetProcAddress(instance->libraryHandle, "fmi2" #f); \
    if (!instance->fmi2Functions->fmi2 ## f) { \
        instance->logMessage(instance, FMIFatal, "fatal", "Symbol fmi2" #f " is missing in shared library."); \
        return FMIFatal; \
    }\
} while (0)
#else
#define LOAD_SYMBOL(f) \
do { \
    instance->fmi2Functions->fmi2 ## f = (fmi2 ## f ## TYPE*)dlsym(instance->libraryHandle, "fmi2" #f); \
    if (!instance->fmi2Functions->fmi2 ## f) { \
        instance->logMessage(instance, FMIFatal, "fatal", "Symbol fmi2" #f " is missing in shared library."); \
        return FMIFatal; \
    } \
} while (0)
#endif

#define CALL(f) \
do { \
    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2 ## f (instance->component); \
    if (instance->logFunctionCall) { \
        instance->logFunctionCall(instance, status, "fmi2" #f "()"); \
    } \
    return status; \
} while (0)

#define CALL_ARGS(f, m, ...) \
do { \
    const FMIStatus status = (FMIStatus)instance->fmi2Functions-> fmi2 ## f (instance->component, __VA_ARGS__); \
    if (instance->logFunctionCall) { \
        FMIClearLogMessageBuffer(instance); \
        FMIAppendToLogMessageBuffer(instance, "fmi2" #f "(" m ")", __VA_ARGS__); \
        instance->logFunctionCall(instance, status, instance->logMessageBuffer); \
    } \
    return status; \
} while (0)

#define CALL_ARRAY(s, t) \
do { \
    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2 ## s ## t(instance->component, vr, nvr, value); \
    if (instance->logFunctionCall) { \
        FMIClearLogMessageBuffer(instance); \
        FMIAppendToLogMessageBuffer(instance, "fmi2" #s #t "(vr={"); \
        FMIAppendArrayToLogMessageBuffer(instance, vr, nvr, NULL, FMIValueReferenceType); \
        FMIAppendToLogMessageBuffer(instance, "}, nvr=%zu, value={", nvr); \
        FMIAppendArrayToLogMessageBuffer(instance, value, nvr, NULL, FMI ## t ## Type); \
        FMIAppendToLogMessageBuffer(instance, "})"); \
        instance->logFunctionCall(instance, status, instance->logMessageBuffer); \
    } \
    return status; \
} while (0)

/***************************************************
Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* FMI2GetTypesPlatform(FMIInstance *instance) {

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmi2GetTypesPlatform()");
    }

    return instance->fmi2Functions->fmi2GetTypesPlatform();
}

const char* FMI2GetVersion(FMIInstance *instance) {

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmi2GetVersion()");
    }

    return instance->fmi2Functions->fmi2GetVersion();
}

FMIStatus FMI2SetDebugLogging(FMIInstance *instance, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2SetDebugLogging(instance->component, loggingOn, nCategories, categories);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2SetDebugLogging(loggingOn=%d, nCategories=%zu, categories={");
        FMIAppendArrayToLogMessageBuffer(instance, categories, nCategories, NULL, FMIStringType);
        FMIAppendToLogMessageBuffer(instance, "})");
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/* Creation and destruction of FMU instances and setting debug status */
FMIStatus FMI2Instantiate(FMIInstance *instance, const char *fmuResourceLocation, fmi2Type fmuType, fmi2String fmuGUID,
    fmi2Boolean visible, fmi2Boolean loggingOn) {

    instance->fmiMajorVersion = FMIMajorVersion2;

    instance->fmi2Functions = calloc(1, sizeof(FMI2Functions));

    if (!instance->fmi2Functions) {
        return FMIError;
    }

    instance->state = FMIStartAndEndState;

#if !defined(FMI_VERSION) || FMI_VERSION == 2

    /***************************************************
    Common Functions
    ****************************************************/

    /* required functions */
    LOAD_SYMBOL(GetTypesPlatform);
    LOAD_SYMBOL(GetVersion);
    LOAD_SYMBOL(SetDebugLogging);
    LOAD_SYMBOL(Instantiate);
    LOAD_SYMBOL(FreeInstance);
    LOAD_SYMBOL(SetupExperiment);
    LOAD_SYMBOL(EnterInitializationMode);
    LOAD_SYMBOL(ExitInitializationMode);
    LOAD_SYMBOL(Terminate);
    LOAD_SYMBOL(Reset);
    LOAD_SYMBOL(GetReal);
    LOAD_SYMBOL(GetInteger);
    LOAD_SYMBOL(GetBoolean);
    LOAD_SYMBOL(GetString);
    LOAD_SYMBOL(SetReal);
    LOAD_SYMBOL(SetInteger);
    LOAD_SYMBOL(SetBoolean);
    LOAD_SYMBOL(SetString);

    /* optional functions */
    LOAD_SYMBOL(GetFMUstate);
    LOAD_SYMBOL(SetFMUstate);
    LOAD_SYMBOL(FreeFMUstate);
    LOAD_SYMBOL(SerializedFMUstateSize);
    LOAD_SYMBOL(SerializeFMUstate);
    LOAD_SYMBOL(DeSerializeFMUstate);
    LOAD_SYMBOL(GetDirectionalDerivative);

    if (fmuType == fmi2ModelExchange) {
#ifndef CO_SIMULATION
        /***************************************************
        Model Exchange
        ****************************************************/

        LOAD_SYMBOL(EnterEventMode);
        LOAD_SYMBOL(NewDiscreteStates);
        LOAD_SYMBOL(EnterContinuousTimeMode);
        LOAD_SYMBOL(CompletedIntegratorStep);
        LOAD_SYMBOL(SetTime);
        LOAD_SYMBOL(SetContinuousStates);
        LOAD_SYMBOL(GetDerivatives);
        LOAD_SYMBOL(GetEventIndicators);
        LOAD_SYMBOL(GetContinuousStates);
        LOAD_SYMBOL(GetNominalsOfContinuousStates);
#endif
    } else {
#ifndef MODEL_EXCHANGE
        /***************************************************
        Co-Simulation
        ****************************************************/

        LOAD_SYMBOL(SetRealInputDerivatives);
        LOAD_SYMBOL(GetRealOutputDerivatives);
        LOAD_SYMBOL(DoStep);
        LOAD_SYMBOL(CancelStep);
        LOAD_SYMBOL(GetStatus);
        LOAD_SYMBOL(GetRealStatus);
        LOAD_SYMBOL(GetIntegerStatus);
        LOAD_SYMBOL(GetBooleanStatus);
        LOAD_SYMBOL(GetStringStatus);
#endif
    }

#endif

    instance->fmi2Functions->callbacks.logger               = cb_logMessage2;
    instance->fmi2Functions->callbacks.allocateMemory       = calloc;
    instance->fmi2Functions->callbacks.freeMemory           = free;
    instance->fmi2Functions->callbacks.stepFinished         = NULL;
    instance->fmi2Functions->callbacks.componentEnvironment = instance;

    instance->component = instance->fmi2Functions->fmi2Instantiate(instance->name, fmuType, fmuGUID, fmuResourceLocation, &instance->fmi2Functions->callbacks, visible, loggingOn);

    if (instance->logFunctionCall) {
        fmi2CallbackFunctions* f = &instance->fmi2Functions->callbacks;
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2Instantiate(instanceName=\"%s\", fmuType=%d, fmuGUID=\"%s\", fmuResourceLocation=\"%s\", functions={logger=0x%p, allocateMemory=0x%p, freeMemory=0x%p, stepFinished=0x%p, componentEnvironment=0x%p}, visible=%d, loggingOn=%d)",
            instance->name, fmuType, fmuGUID, fmuResourceLocation, f->logger, f->allocateMemory, f->freeMemory, f->stepFinished, f->componentEnvironment, visible, loggingOn);
        instance->logFunctionCall(instance, instance->component ? FMIOK : FMIError, instance->logMessageBuffer);
    }

    if (!instance->component) {
        return FMIError;
    }

    instance->interfaceType = (FMIInterfaceType)fmuType;
    instance->state = FMIInstantiatedState;

    return FMIOK;
}

void FMI2FreeInstance(FMIInstance *instance) {

    if (!instance) {
        return;
    }

    instance->fmi2Functions->fmi2FreeInstance(instance->component);

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmi2FreeInstance()");
    }
}

/* Enter and exit initialization mode, terminate and reset */
FMIStatus FMI2SetupExperiment(FMIInstance *instance,
    fmi2Boolean toleranceDefined,
    fmi2Real tolerance,
    fmi2Real startTime,
    fmi2Boolean stopTimeDefined,
    fmi2Real stopTime) {
    CALL_ARGS(SetupExperiment, "toleranceDefined=%d, tolerance=%.16g, startTime=%.16g, stopTimeDefined=%d, stopTime=%.16g", toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
}

FMIStatus FMI2EnterInitializationMode(FMIInstance *instance) {
    instance->state = FMIInitializationModeState;
    CALL(EnterInitializationMode);
}

FMIStatus FMI2ExitInitializationMode(FMIInstance *instance) {
    instance->state = instance->interfaceType == FMIModelExchange ? FMIEventModeState : FMIStepModeState;
    CALL(ExitInitializationMode);
}

FMIStatus FMI2Terminate(FMIInstance *instance) {
    instance->state = FMITerminatedState;
    CALL(Terminate);
}

FMIStatus FMI2Reset(FMIInstance *instance) {
    instance->state = FMIInstantiatedState;
    CALL(Reset);
}

/* Getting and setting variable values */
FMIStatus FMI2GetReal(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    CALL_ARRAY(Get, Real);
}

FMIStatus FMI2GetInteger(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    CALL_ARRAY(Get, Integer);
}

FMIStatus FMI2GetBoolean(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    CALL_ARRAY(Get, Boolean);
}

FMIStatus FMI2GetString(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    CALL_ARRAY(Get, String);
}

FMIStatus FMI2SetReal(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    CALL_ARRAY(Set, Real);
}

FMIStatus FMI2SetInteger(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    CALL_ARRAY(Set, Integer);
}

FMIStatus FMI2SetBoolean(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    CALL_ARRAY(Set, Boolean);
}

FMIStatus FMI2SetString(FMIInstance *instance, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    CALL_ARRAY(Set, String);
}

/* Getting and setting the internal FMU state */
FMIStatus FMI2GetFMUstate(FMIInstance *instance, fmi2FMUstate* FMUstate) {
    CALL_ARGS(GetFMUstate, "FMUstate=0x%p", FMUstate);
}

FMIStatus FMI2SetFMUstate(FMIInstance *instance, fmi2FMUstate  FMUstate) {
    CALL_ARGS(SetFMUstate, "FMUstate=0x%p", FMUstate);
}

FMIStatus FMI2FreeFMUstate(FMIInstance *instance, fmi2FMUstate* FMUstate) {
    CALL_ARGS(FreeFMUstate, "FMUstate=0x%p", FMUstate);
}

FMIStatus FMI2SerializedFMUstateSize(FMIInstance *instance, fmi2FMUstate  FMUstate, size_t* size) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2SerializedFMUstateSize(instance->component, FMUstate, size);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2SerializedFMUstateSize(FMUstate=0x%p, size=%zu)", FMUstate, *size);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2SerializeFMUstate(FMIInstance *instance, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
    CALL_ARGS(SerializeFMUstate, "FMUstate=0x%p, serializedState=0x%p, size=%zu", FMUstate, serializedState, size);
}

FMIStatus FMI2DeSerializeFMUstate(FMIInstance *instance, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    CALL_ARGS(DeSerializeFMUstate, "serializedState=0x%p, size=%zu, FMUstate=0x%p", serializedState, size, FMUstate);
}

/* Getting partial derivatives */
FMIStatus FMI2GetDirectionalDerivative(FMIInstance *instance,
    const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
    const fmi2ValueReference vKnown_ref[], size_t nKnown,
    const fmi2Real dvKnown[],
    fmi2Real dvUnknown[]) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetDirectionalDerivative(instance->component, vUnknown_ref, nUnknown, vKnown_ref, nKnown, dvKnown, dvUnknown);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetDirectionalDerivative(vUnknown_ref={");
        FMIAppendArrayToLogMessageBuffer(instance, vUnknown_ref, nUnknown, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nUnknown=%zu, vKnown_ref={", nUnknown);
        FMIAppendArrayToLogMessageBuffer(instance, vKnown_ref, nKnown, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nKnown=%zu, dvKnown={", nKnown);
        FMIAppendArrayToLogMessageBuffer(instance, dvKnown, nKnown, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, dvUnknown={", nKnown);
        FMIAppendArrayToLogMessageBuffer(instance, dvUnknown, nUnknown, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "})");
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/***************************************************
Model Exchange
****************************************************/

/* Enter and exit the different modes */
FMIStatus FMI2EnterEventMode(FMIInstance *instance) {
    instance->state = FMIEventModeState;
    CALL(EnterEventMode);
}

FMIStatus FMI2NewDiscreteStates(FMIInstance *instance, fmi2EventInfo *eventInfo) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2NewDiscreteStates(instance->component, eventInfo);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi2NewDiscreteStates(eventInfo={newDiscreteStatesNeeded=%d, terminateSimulation=%d, nominalsOfContinuousStatesChanged=%d, valuesOfContinuousStatesChanged=%d, nextEventTimeDefined=%d, nextEventTime=%.16g})",
            eventInfo->newDiscreteStatesNeeded, eventInfo->terminateSimulation, eventInfo->nominalsOfContinuousStatesChanged, eventInfo->valuesOfContinuousStatesChanged, eventInfo->nextEventTimeDefined, eventInfo->nextEventTime);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2EnterContinuousTimeMode(FMIInstance *instance) {
    instance->state = FMIContinuousTimeModeState;
    CALL(EnterContinuousTimeMode);
}

FMIStatus FMI2CompletedIntegratorStep(FMIInstance *instance,
    fmi2Boolean   noSetFMUStatePriorToCurrentPoint,
    fmi2Boolean*  enterEventMode,
    fmi2Boolean*  terminateSimulation) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2CompletedIntegratorStep(instance->component, noSetFMUStatePriorToCurrentPoint, enterEventMode, terminateSimulation);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmi2CompletedIntegratorStep(noSetFMUStatePriorToCurrentPoint=%d, enterEventMode=%d, terminateSimulation=%d)",
            noSetFMUStatePriorToCurrentPoint, *enterEventMode, *terminateSimulation);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/* Providing independent variables and re-initialization of caching */
FMIStatus FMI2SetTime(FMIInstance* instance, fmi2Real time) {
    CALL_ARGS(SetTime, "time=%.16g", time);
}

FMIStatus FMI2SetContinuousStates(FMIInstance *instance, const fmi2Real x[], size_t nx) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2SetContinuousStates(instance->component, x, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2SetContinuousStates(x={");
        FMIAppendArrayToLogMessageBuffer(instance, x, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/* Evaluation of the model equations */
FMIStatus FMI2GetDerivatives(FMIInstance *instance, fmi2Real derivatives[], size_t nx) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetDerivatives(instance->component, derivatives, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetDerivatives(derivatives={");
        FMIAppendArrayToLogMessageBuffer(instance, derivatives, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetEventIndicators(FMIInstance *instance, fmi2Real eventIndicators[], size_t ni) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetEventIndicators(instance->component, eventIndicators, ni);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetEventIndicators(eventIndicators={");
        FMIAppendArrayToLogMessageBuffer(instance, eventIndicators, ni, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, ni=%zu)", ni);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetContinuousStates(FMIInstance *instance, fmi2Real x[], size_t nx) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetContinuousStates(instance->component, x, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetContinuousStates(x={");
        FMIAppendArrayToLogMessageBuffer(instance, x, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetNominalsOfContinuousStates(FMIInstance *instance, fmi2Real x_nominal[], size_t nx) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetNominalsOfContinuousStates(instance->component, x_nominal, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetNominalsOfContinuousStates(x_nominal={");
        FMIAppendArrayToLogMessageBuffer(instance, x_nominal, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

/***************************************************
Co-Simulation
****************************************************/

/* Simulating the slave */
FMIStatus FMI2SetRealInputDerivatives(FMIInstance *instance,
    const fmi2ValueReference vr[], size_t nvr,
    const fmi2Integer order[],
    const fmi2Real value[]) {
    CALL_ARGS(SetRealInputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value);
}

FMIStatus FMI2GetRealOutputDerivatives(FMIInstance *instance,
    const fmi2ValueReference vr[], size_t nvr,
    const fmi2Integer order[],
    fmi2Real value[]) {
    CALL_ARGS(GetRealOutputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value);
}

FMIStatus FMI2DoStep(FMIInstance *instance,
    fmi2Real      currentCommunicationPoint,
    fmi2Real      communicationStepSize,
    fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {
    CALL_ARGS(DoStep, "currentCommunicationPoint=%.16g, communicationStepSize=%.16g, noSetFMUStatePriorToCurrentPoint=%d",
        currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);
}

FMIStatus FMI2CancelStep(FMIInstance *instance) {
    CALL(CancelStep);
}

/* Inquire slave status */
FMIStatus FMI2GetStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Status* value) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetStatus(instance->component, s, value);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetStatus(s=%d, value=%d)", s, *value);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetRealStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Real* value) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetRealStatus(instance->component, s, value);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetRealStatus(s=%d, value=%.16g)", s, *value);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetIntegerStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Integer* value) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetIntegerStatus(instance->component, s, value);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetIntegerStatus(s=%d, value=%d)", s, *value);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetBooleanStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2Boolean* value) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetBooleanStatus(instance->component, s, value);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetBooleanStatus(s=%d, value=%d)", s, *value);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI2GetStringStatus(FMIInstance *instance, const fmi2StatusKind s, fmi2String* value) {

    const FMIStatus status = (FMIStatus)instance->fmi2Functions->fmi2GetStringStatus(instance->component, s, value);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmi2GetStringStatus(s=%d, value=\"%s\")", s, *value);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

#undef LOAD_SYMBOL
#undef CALL
#undef CALL_ARGS
#undef CALL_ARRAY
