#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include "FMI1.h"


// keep track of the current instance for the log callback
static FMIInstance *currentInstance = NULL;

static void cb_logMessage1(fmi1Component c, fmi1String instanceName, fmi1Status status, fmi1String category, fmi1String message, ...) {

    if (!currentInstance) return;

    char buf[FMI_MAX_MESSAGE_LENGTH];

    va_list args;

    va_start(args, message);
    vsnprintf(buf, FMI_MAX_MESSAGE_LENGTH, message, args);
    va_end(args);

    if (!currentInstance->logMessage) return;

    currentInstance->logMessage(currentInstance, (FMIStatus)status, category, buf);
}

#define MAX_SYMBOL_LENGTH 256

static void *loadSymbol(FMIInstance *instance, const char *prefix, const char *name) {
    char fname[MAX_SYMBOL_LENGTH];
    strcpy(fname, prefix);
    strcat(fname, name);
#ifdef _WIN32
    void *addr = GetProcAddress(instance->libraryHandle, fname);
#else
    void *addr = dlsym(instance->libraryHandle, fname);
#endif
    if (!addr) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "Failed to load function \"%s\".", fname);
        instance->logFunctionCall(instance, FMIError, instance->logMessageBuffer);
    }
    return addr;
}

#define LOAD_SYMBOL(f) \
do { \
    instance->fmi1Functions->fmi1 ## f = (fmi1 ## f ## TYPE*)loadSymbol(instance, modelIdentifier, "_fmi" #f); \
    if (!instance->fmi1Functions->fmi1 ## f) { \
        status = FMIError; \
        goto fail; \
    } \
} while (0)

#define CALL(f) \
do { \
    currentInstance = instance; \
    FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1 ## f (instance->component); \
    if (instance->logFunctionCall) { \
        instance->logFunctionCall(instance, status, "fmi" #f "()"); \
    } \
    return status; \
} while (0)

#define CALL_ARGS(f, m, ...) \
do { \
    currentInstance = instance; \
    FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1 ## f (instance->component, __VA_ARGS__); \
    if (instance->logFunctionCall) { \
        FMIClearLogMessageBuffer(instance); \
        FMIAppendToLogMessageBuffer(instance, "fmi" #f "(" m ")", __VA_ARGS__); \
        instance->logFunctionCall(instance, status, instance->logMessageBuffer); \
    } \
    return status; \
} while (0)

#define CALL_ARRAY(s, t) \
do { \
    currentInstance = instance; \
    FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1 ## s ## t(instance->component, vr, nvr, value); \
    if (instance->logFunctionCall) { \
        FMIClearLogMessageBuffer(instance); \
        FMIAppendToLogMessageBuffer(instance, "fmi" #s #t "(vr={"); \
        FMIAppendArrayToLogMessageBuffer(instance, vr, nvr, NULL, FMIValueReferenceType); \
        FMIAppendToLogMessageBuffer(instance, "}, nvr=%zu, value={", nvr); \
        FMIAppendArrayToLogMessageBuffer(instance, value, nvr, NULL, FMI ## t ## Type); \
        FMIAppendToLogMessageBuffer(instance, "})"); \
        instance->logFunctionCall(instance, status, instance->logMessageBuffer); \
    } \
    return status; \
} while (0)

/***************************************************
 Common Functions for FMI 1.0
****************************************************/

FMIStatus FMI1SetReal(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Real    value[]) {
    CALL_ARRAY(Set, Real);
}

FMIStatus FMI1SetInteger(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer value[]) {
    CALL_ARRAY(Set, Integer);
}

FMIStatus FMI1SetBoolean(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Boolean value[]) {
    CALL_ARRAY(Set, Boolean);
}

FMIStatus FMI1SetString(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1String  value[]) {
    CALL_ARRAY(Set, String);
}

FMIStatus FMI1GetReal(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1Real    value[]) {
    CALL_ARRAY(Get, Real);
}

FMIStatus FMI1GetInteger(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1Integer value[]) {
    CALL_ARRAY(Get, Integer);
}

FMIStatus FMI1GetBoolean(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1Boolean value[]) {
    CALL_ARRAY(Get, Boolean);
}

FMIStatus FMI1GetString(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, fmi1String  value[]) {
    CALL_ARRAY(Get, String);
}

FMIStatus FMI1SetDebugLogging(FMIInstance *instance, fmi1Boolean loggingOn) {
    CALL_ARGS(SetDebugLogging, "loggingOn=%d", loggingOn);
}


/***************************************************
 FMI 1.0 for Model Exchange Functions
****************************************************/

const char* FMI1GetModelTypesPlatform(FMIInstance *instance) {
    currentInstance = instance;
    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmiGetModelTypesPlatform()");
    }
    return instance->fmi1Functions->fmi1GetModelTypesPlatform();
}

const char* FMI1GetVersion(FMIInstance *instance) {
    currentInstance = instance;
    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmiGetVersion()");
    }
    return instance->fmi1Functions->fmi1GetVersion();
}

FMIStatus FMI1InstantiateModel(FMIInstance *instance, fmi1String modelIdentifier, fmi1String GUID, fmi1Boolean loggingOn) {

    currentInstance = instance;

    instance->fmiVersion = FMIVersion1;

    instance->interfaceType = FMIModelExchange;

    FMIStatus status = FMIOK;

    instance->fmi1Functions = calloc(1, sizeof(FMI1Functions));

    if (!instance->fmi1Functions) {
        return FMIError;
    }

    /***************************************************
     Common Functions for FMI 1.0
    ****************************************************/
    LOAD_SYMBOL(SetReal);
    LOAD_SYMBOL(SetInteger);
    LOAD_SYMBOL(SetBoolean);
    LOAD_SYMBOL(SetString);
    LOAD_SYMBOL(GetReal);
    LOAD_SYMBOL(GetInteger);
    LOAD_SYMBOL(GetBoolean);
    LOAD_SYMBOL(GetString);
    LOAD_SYMBOL(SetDebugLogging);

    /***************************************************
        FMI 1.0 for Model Exchange Functions
    ****************************************************/
    LOAD_SYMBOL(GetModelTypesPlatform);
    LOAD_SYMBOL(GetVersion);
    LOAD_SYMBOL(InstantiateModel);
    LOAD_SYMBOL(FreeModelInstance);
    LOAD_SYMBOL(SetTime);
    LOAD_SYMBOL(SetContinuousStates);
    LOAD_SYMBOL(CompletedIntegratorStep);
    LOAD_SYMBOL(Initialize);
    LOAD_SYMBOL(GetDerivatives);
    LOAD_SYMBOL(GetEventIndicators);
    LOAD_SYMBOL(EventUpdate);
    LOAD_SYMBOL(GetContinuousStates);
    LOAD_SYMBOL(GetNominalContinuousStates);
    LOAD_SYMBOL(GetStateValueReferences);
    LOAD_SYMBOL(Terminate);

    instance->fmi1Functions->callbacks.logger         = cb_logMessage1;
    instance->fmi1Functions->callbacks.allocateMemory = calloc;
    instance->fmi1Functions->callbacks.freeMemory     = free;
    instance->fmi1Functions->callbacks.stepFinished   = NULL;

    instance->component = instance->fmi1Functions->fmi1InstantiateModel(instance->name, GUID, instance->fmi1Functions->callbacks, loggingOn);

    status = instance->component ? FMIOK : FMIError;

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmiInstantiateModel(instanceName=\"%s\", GUID=\"%s\", functions=0x%p, loggingOn=%d)",
            instance->name, GUID, &instance->fmi1Functions->callbacks, loggingOn);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

fail:
    return status;
}

void FMI1FreeModelInstance(FMIInstance *instance) {

    if (!instance) {
        return;
    }

    currentInstance = instance;

    instance->fmi1Functions->fmi1FreeModelInstance(instance->component);

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmiFreeModelInstance()");
    }
}

FMIStatus FMI1SetTime(FMIInstance *instance, fmi1Real time) {
    CALL_ARGS(SetTime, "time=%.16g", time);
}

FMIStatus FMI1SetContinuousStates(FMIInstance *instance, const fmi1Real x[], size_t nx) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1SetContinuousStates(instance->component, x, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiSetContinuousStates(x={");
        FMIAppendArrayToLogMessageBuffer(instance, x, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1CompletedIntegratorStep(FMIInstance *instance, fmi1Boolean* callEventUpdate) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1CompletedIntegratorStep(instance->component, callEventUpdate);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiCompletedIntegratorStep(callEventUpdate=%d)", *callEventUpdate);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1Initialize(FMIInstance *instance, fmi1Boolean toleranceControlled, fmi1Real relativeTolerance, fmi1EventInfo* eventInfo) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1Initialize(instance->component, toleranceControlled, relativeTolerance, eventInfo);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmiInitialize(toleranceControlled=%d, relativeTolerance=%.16g, eventInfo={iterationConverged=%d, stateValueReferencesChanged=%d, stateValuesChanged=%d, terminateSimulation=%d, upcomingTimeEvent=%d, nextEventTime=%.16g})",
            toleranceControlled, relativeTolerance, eventInfo->iterationConverged, eventInfo->stateValueReferencesChanged, eventInfo->stateValuesChanged, eventInfo->terminateSimulation, eventInfo->upcomingTimeEvent, eventInfo->nextEventTime);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1GetDerivatives(FMIInstance *instance, fmi1Real derivatives[], size_t nx) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1GetDerivatives(instance->component, derivatives, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiGetDerivatives(derivatives={");
        FMIAppendArrayToLogMessageBuffer(instance, derivatives, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1GetEventIndicators(FMIInstance *instance, fmi1Real eventIndicators[], size_t ni) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1GetEventIndicators(instance->component, eventIndicators, ni);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiGetEventIndicators(eventIndicators={");
        FMIAppendArrayToLogMessageBuffer(instance, eventIndicators, ni, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, ni=%zu)", ni);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1EventUpdate(FMIInstance *instance, fmi1Boolean intermediateResults, fmi1EventInfo* eventInfo) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1EventUpdate(instance->component, intermediateResults, eventInfo);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmiEventUpdate(intermediateResults=%d, eventInfo={iterationConverged=%d, stateValueReferencesChanged=%d, stateValuesChanged=%d, terminateSimulation=%d, upcomingTimeEvent=%d, nextEventTime=%.16g})",
            intermediateResults, eventInfo->iterationConverged, eventInfo->stateValueReferencesChanged, eventInfo->stateValuesChanged, eventInfo->terminateSimulation, eventInfo->upcomingTimeEvent, eventInfo->nextEventTime);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1GetContinuousStates(FMIInstance *instance, fmi1Real states[], size_t nx) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1GetContinuousStates(instance->component, states, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiGetContinuousStates(states={");
        FMIAppendArrayToLogMessageBuffer(instance, states, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1GetNominalContinuousStates(FMIInstance *instance, fmi1Real x_nominal[], size_t nx) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1GetNominalContinuousStates(instance->component, x_nominal, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiGetNominalContinuousStates(x_nominal={");
        FMIAppendArrayToLogMessageBuffer(instance, x_nominal, nx, NULL, FMIRealType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1GetStateValueReferences(FMIInstance *instance, fmi1ValueReference vrx[], size_t nx) {

    currentInstance = instance;

    const FMIStatus status = (FMIStatus)instance->fmi1Functions->fmi1GetStateValueReferences(instance->component, vrx, nx);

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance, "fmiGetStateValueReferences(vrx={");
        FMIAppendArrayToLogMessageBuffer(instance, vrx, nx, NULL, FMIValueReferenceType);
        FMIAppendToLogMessageBuffer(instance, "}, nx=%zu)", nx);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

    return status;
}

FMIStatus FMI1Terminate(FMIInstance *instance) {
    CALL(Terminate);
}

/***************************************************
 FMI 1.0 for Co-Simulation Functions
****************************************************/

const char*   FMI1GetTypesPlatform(FMIInstance *instance) {

    currentInstance = instance;

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmiGetTypesPlatform()");
    }

    return instance->fmi1Functions->fmi1GetTypesPlatform();
}

FMIStatus FMI1InstantiateSlave(FMIInstance *instance, fmi1String modelIdentifier, fmi1String fmuGUID, fmi1String fmuLocation, fmi1String  mimeType, fmi1Real timeout, fmi1Boolean visible, fmi1Boolean interactive, fmi1Boolean loggingOn) {

    currentInstance = instance;

    instance->fmiVersion = FMIVersion1;

    instance->interfaceType = FMICoSimulation;

    FMIStatus status = FMIOK;

    instance->fmi1Functions = calloc(1, sizeof(FMI1Functions));

    if (!instance->fmi1Functions) {
        return FMIError;
    }

    /***************************************************
     Common Functions for FMI 1.0
    ****************************************************/
    LOAD_SYMBOL(SetReal);
    LOAD_SYMBOL(SetInteger);
    LOAD_SYMBOL(SetBoolean);
    LOAD_SYMBOL(SetString);
    LOAD_SYMBOL(GetReal);
    LOAD_SYMBOL(GetInteger);
    LOAD_SYMBOL(GetBoolean);
    LOAD_SYMBOL(GetString);
    LOAD_SYMBOL(SetDebugLogging);

    /***************************************************
     FMI 1.0 for Co-Simulation Functions
    ****************************************************/
    LOAD_SYMBOL(GetTypesPlatform);
    LOAD_SYMBOL(InstantiateSlave);
    LOAD_SYMBOL(InitializeSlave);
    LOAD_SYMBOL(TerminateSlave);
    LOAD_SYMBOL(ResetSlave);
    LOAD_SYMBOL(FreeSlaveInstance);
    LOAD_SYMBOL(SetRealInputDerivatives);
    LOAD_SYMBOL(GetRealOutputDerivatives);
    LOAD_SYMBOL(CancelStep);
    LOAD_SYMBOL(DoStep);
    LOAD_SYMBOL(GetStatus);
    LOAD_SYMBOL(GetRealStatus);
    LOAD_SYMBOL(GetIntegerStatus);
    LOAD_SYMBOL(GetBooleanStatus);
    LOAD_SYMBOL(GetStringStatus);

    instance->fmi1Functions->callbacks.logger         = cb_logMessage1;
    instance->fmi1Functions->callbacks.allocateMemory = calloc;
    instance->fmi1Functions->callbacks.freeMemory     = free;
    instance->fmi1Functions->callbacks.stepFinished   = NULL;

    instance->component = instance->fmi1Functions->fmi1InstantiateSlave(instance->name, fmuGUID, fmuLocation, mimeType, timeout, visible, interactive, instance->fmi1Functions->callbacks, loggingOn);

    status = instance->component ? FMIOK : FMIError;

    if (instance->logFunctionCall) {
        FMIClearLogMessageBuffer(instance);
        FMIAppendToLogMessageBuffer(instance,
            "fmiInstantiateSlave(instanceName=\"%s\", fmuGUID=\"%s\", fmuLocation=\"%s\", mimeType=\"%s\", timeout=%.16g, visible=%d, interactive=%d, functions=0x%p, loggingOn=%d)",
            instance->name, fmuGUID, fmuLocation, mimeType, timeout, visible, interactive, &instance->fmi1Functions->callbacks, loggingOn);
        instance->logFunctionCall(instance, status, instance->logMessageBuffer);
    }

fail:
    return status;
}

FMIStatus FMI1InitializeSlave(FMIInstance *instance, fmi1Real tStart, fmi1Boolean stopTimeDefined, fmi1Real tStop) {

    instance->time = tStart;

    CALL_ARGS(InitializeSlave, "tStart=%.16g, stopTimeDefined=%d, tStop=%.16g", tStart, stopTimeDefined, tStop);
}

FMIStatus FMI1TerminateSlave(FMIInstance *instance) {
    CALL(TerminateSlave);
}

FMIStatus FMI1ResetSlave(FMIInstance *instance) {
    CALL(ResetSlave);
}

void FMI1FreeSlaveInstance(FMIInstance *instance) {

    if (!instance) {
        return;
    }

    currentInstance = instance;

    instance->fmi1Functions->fmi1FreeSlaveInstance(instance->component);

    if (instance->logFunctionCall) {
        instance->logFunctionCall(instance, FMIOK, "fmiFreeSlaveInstance()");
    }
}

FMIStatus FMI1SetRealInputDerivatives(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], const fmi1Real value[]) {
    CALL_ARGS(SetRealInputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value);
}

FMIStatus FMI1GetRealOutputDerivatives(FMIInstance *instance, const fmi1ValueReference vr[], size_t nvr, const fmi1Integer order[], fmi1Real value[]) {
    CALL_ARGS(GetRealOutputDerivatives, "vr=0x%p, nvr=%zu, order=0x%p, value=0x%p", vr, nvr, order, value);
}

FMIStatus FMI1CancelStep(FMIInstance *instance) {
    CALL(CancelStep);
}

FMIStatus FMI1DoStep(FMIInstance *instance, fmi1Real currentCommunicationPoint, fmi1Real communicationStepSize, fmi1Boolean newStep) {

    currentInstance = instance;

    instance->time = currentCommunicationPoint + communicationStepSize;

    CALL_ARGS(DoStep, "currentCommunicationPoint=%.16g, communicationStepSize=%.16g, newStep=%d",
        currentCommunicationPoint, communicationStepSize, newStep);
}

FMIStatus FMI1GetStatus(FMIInstance *instance, const fmi1StatusKind s, fmi1Status* value) {
    CALL_ARGS(GetStatus, "s=%d, value=0x%p", s, value);
}

FMIStatus FMI1GetRealStatus(FMIInstance *instance, const fmi1StatusKind s, fmi1Real* value) {
    CALL_ARGS(GetRealStatus, "s=%d, value=0x%p", s, value);
}

FMIStatus FMI1GetIntegerStatus(FMIInstance *instance, const fmi1StatusKind s, fmi1Integer* value) {
    CALL_ARGS(GetIntegerStatus, "s=%d, value=0x%p", s, value);
}

FMIStatus FMI1GetBooleanStatus(FMIInstance *instance, const fmi1StatusKind s, fmi1Boolean* value) {
    CALL_ARGS(GetBooleanStatus, "s=%d, value=0x%p", s, value);
}

FMIStatus FMI1GetStringStatus(FMIInstance *instance, const fmi1StatusKind s, fmi1String* value) {
    CALL_ARGS(GetStringStatus, "s=%d, value=0x%p", s, value);
}

#undef LOAD_SYMBOL
#undef CALL
#undef CALL_ARGS
#undef CALL_ARRAY
