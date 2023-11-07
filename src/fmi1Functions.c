#if FMI_VERSION != 1
#error FMI_VERSION must be 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#include "config.h"
#include "model.h"
#include "cosimulation.h"


#ifdef FMI_COSIMULATION
#include "fmiFunctions.h"
#else
#include "fmiModelFunctions.h"
#endif

#define ASSERT_NOT_NULL(p) \
do { \
    if (!p) { \
        logError(S, "Argument %s must not be NULL.", xstr(p)); \
        S->state = modelError; \
        return (fmiStatus)Error; \
    } \
} while (0)

#define GET_VARIABLES(T) \
do { \
    ASSERT_NOT_NULL(vr); \
    ASSERT_NOT_NULL(value); \
    size_t index = 0; \
    Status status = OK; \
    if (nvr == 0) return (fmiStatus)status; \
    if (S->isDirtyValues) { \
        Status s = calculateValues(S); \
        status = max(status, s); \
        if (status > Warning) return (fmiStatus)status; \
        S->isDirtyValues = false; \
    } \
    for (size_t i = 0; i < nvr; i++) { \
        Status s = get ## T(S, vr[i], value, nvr, &index); \
        status = max(status, s); \
        if (status > Warning) return (fmiStatus)status; \
    } \
    return (fmiStatus)status; \
} while (0)

#define SET_VARIABLES(T) \
do { \
    ASSERT_NOT_NULL(vr); \
    ASSERT_NOT_NULL(value); \
    size_t index = 0; \
    Status status = OK; \
    for (size_t i = 0; i < nvr; i++) { \
        Status s = set ## T(S, vr[i], value, nvr, &index); \
        status = max(status, s); \
        if (status > Warning) return (fmiStatus)status; \
    } \
    if (nvr > 0) S->isDirtyValues = true; \
    return (fmiStatus)status; \
} while (0)

#define GET_BOOLEAN_VARIABLES \
do { \
    Status status = OK; \
    for (size_t i = 0; i < nvr; i++) { \
        bool v = false; \
        size_t index = 0; \
        Status s = getBoolean(S, vr[i], &v, nvr, &index); \
        value[i] = v; \
        status = max(status, s); \
        if (status > Warning) return (fmiStatus)status; \
    } \
    return (fmiStatus)status; \
} while (0)

#define SET_BOOLEAN_VARIABLES \
do { \
    Status status = OK; \
    for (size_t i = 0; i < nvr; i++) { \
        bool v = value[i]; \
        size_t index = 0; \
        Status s = setBoolean(S, vr[i], &v, nvr, &index); \
        status = max(status, s); \
        if (status > Warning) return (fmiStatus)status; \
    } \
    return (fmiStatus)status; \
} while (0)

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

#define ASSERT_STATE(F, A) \
    if (!c) \
        return fmiError; \
    ModelInstance* S = (ModelInstance *)c; \
    if (invalidState(S, F, not_modelError)) \
        return fmiError;

// ---------------------------------------------------------------------------
// Private helpers used below to implement functions
// ---------------------------------------------------------------------------

// fname is fmiInitialize or fmiInitializeSlave
static fmiStatus init(fmiComponent c) {
    ModelInstance* instance = (ModelInstance *)c;
    instance->state = Initialized;
    calculateValues(instance);
    return fmiOK;
}

// fname is fmiTerminate or fmiTerminateSlave
static fmiStatus terminate(char* fname, fmiComponent c) {
    ModelInstance* instance = (ModelInstance *)c;
    if (invalidState(instance, fname, Initialized))
         return fmiError;
    instance->state = Terminated;
    return fmiOK;
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmiGetVersion() {
    return fmiVersion;
}

// ---------------------------------------------------------------------------
// FMI functions: for FMI Model Exchange 1.0 and for FMI Co-Simulation 1.0
// logging control, setters and getters for Real, Integer, Boolean, String
// ---------------------------------------------------------------------------

fmiStatus fmiSetDebugLogging(fmiComponent c, fmiBoolean loggingOn) {
    ASSERT_STATE("fmiSetDebugLogging", not_modelError);
    return (fmiStatus)setDebugLogging(S, loggingOn, 0, NULL);
}

fmiStatus fmiSetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiReal value[]) {
    ASSERT_STATE("fmiSetReal", Instantiated | Initialized);
    SET_VARIABLES(Float64);
}

fmiStatus fmiSetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger value[]) {
    ASSERT_STATE("fmiSetInteger", Instantiated | Initialized);
    SET_VARIABLES(Int32);
}

fmiStatus fmiSetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiBoolean value[]){
    ASSERT_STATE("fmiSetBoolean", Instantiated | Initialized);
    SET_BOOLEAN_VARIABLES;
}

fmiStatus fmiSetString(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiString value[]){
    ASSERT_STATE("fmiSetString", not_modelError);
    SET_VARIABLES(String);
}

fmiStatus fmiGetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiReal value[]) {
    ASSERT_STATE("fmiGetReal", not_modelError);
    GET_VARIABLES(Float64);
}

fmiStatus fmiGetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiInteger value[]) {
    ASSERT_STATE("fmiGetInteger", not_modelError);
    GET_VARIABLES(Int32);
}

fmiStatus fmiGetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiBoolean value[]) {
    ASSERT_STATE("fmiGetBoolean", not_modelError);
    GET_BOOLEAN_VARIABLES;
}

fmiStatus fmiGetString(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiString  value[]) {
    ASSERT_STATE("fmiGetString", not_modelError);
    GET_VARIABLES(String);
}

#ifdef FMI_COSIMULATION
// ---------------------------------------------------------------------------
// FMI functions: only for FMI Co-Simulation 1.0
// ---------------------------------------------------------------------------

const char* fmiGetTypesPlatform() {
    return fmiPlatform;
}

fmiComponent fmiInstantiateSlave(fmiString  instanceName, fmiString GUID,
    fmiString fmuLocation, fmiString mimeType, fmiReal timeout, fmiBoolean visible,
    fmiBoolean interactive, fmiCallbackFunctions functions, fmiBoolean loggingOn) {

    UNUSED(mimeType);
    UNUSED(timeout);
    UNUSED(visible);
    UNUSED(interactive);

    if (!functions.logger) {
        return NULL;
    }

    // ignoring arguments: mimeType, timeout, visible, interactive
    return createModelInstance(
        (loggerType)functions.logger,
        NULL,
        NULL,
        instanceName,
        GUID,
        fmuLocation,
        loggingOn,
        CoSimulation);
}

fmiStatus fmiInitializeSlave(fmiComponent c, fmiReal tStart, fmiBoolean StopTimeDefined, fmiReal tStop) {

    ModelInstance* instance = (ModelInstance*)c;

    instance->startTime = tStart;
    instance->stopTime = StopTimeDefined ? tStop : INFINITY;
    instance->time = tStart;

    return init(c);
}

fmiStatus fmiTerminateSlave(fmiComponent c) {
    return terminate("fmiTerminateSlave", c);
}

fmiStatus fmiResetSlave(fmiComponent c) {
    ModelInstance* instance = (ModelInstance *)c;
    if (invalidState(instance, "fmiResetSlave", Initialized))
         return fmiError;
    reset(instance);
    return fmiOK;
}

void fmiFreeSlaveInstance(fmiComponent c) {
    freeModelInstance((ModelInstance*)c);
}

fmiStatus fmiSetRealInputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr,
    const fmiInteger order[], const fmiReal value[]) {

    UNUSED(vr);
    UNUSED(nvr);
    UNUSED(order);
    UNUSED(value);

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiSetRealInputDerivatives", Initialized))
         return fmiError;

    logError(instance, "fmiSetRealInputDerivatives: This model cannot interpolate inputs: canInterpolateInputs=\"fmiFalse\"");

    return fmiError;
}

fmiStatus fmiGetRealOutputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger order[], fmiReal value[]) {

    UNUSED(vr);
    UNUSED(nvr);
    UNUSED(order);
    UNUSED(value);

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiGetRealOutputDerivatives", Initialized))
         return fmiError;

    logError(instance, "fmiGetRealOutputDerivatives: This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"");

    return fmiError;
}

fmiStatus fmiCancelStep(fmiComponent c) {

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiCancelStep", Initialized))
         return fmiError;

    logError(instance, "fmiCancelStep: Can be called when fmiDoStep returned fmiPending."
        " This is not the case.");

    return fmiError;
}

fmiStatus fmiDoStep(fmiComponent c, fmiReal currentCommunicationPoint, fmiReal communicationStepSize, fmiBoolean newStep) {

    UNUSED(newStep);

    ModelInstance* instance = (ModelInstance *)c;

    if (currentCommunicationPoint + communicationStepSize > instance->stopTime + EPSILON) {
        logError(instance, "At communication point %.16g a step size of %.16g was requested but stop time is %.16g.",
            currentCommunicationPoint, communicationStepSize, instance->stopTime);
        instance->state = modelError;
        return fmiError;
    }

    const fmiReal nextCommunicationPoint = currentCommunicationPoint + communicationStepSize + EPSILON;

    while (true) {

        if (instance->time + FIXED_SOLVER_STEP > nextCommunicationPoint) {
            break;  // next communcation point reached
        }

        bool stateEvent, timeEvent;

        doFixedStep(instance, &stateEvent, &timeEvent);
#ifdef EVENT_UPDATE
        if (stateEvent || timeEvent) {
            eventUpdate(instance);
        }
#endif
    }

    return fmiOK;
}

fmiStatus fmiGetStatus(fmiComponent c, const fmiStatusKind s, fmiStatus* value) {

    UNUSED(s);
    UNUSED(value);

    logError((ModelInstance*)c, "Not implemented.");

    return fmiError;
}

fmiStatus fmiGetRealStatus(fmiComponent c, const fmiStatusKind s, fmiReal* value){

    UNUSED(c);
    UNUSED(s);
    UNUSED(value);

    logError((ModelInstance*)c, "Not implemented.");

    return fmiError;
}

fmiStatus fmiGetIntegerStatus(fmiComponent c, const fmiStatusKind s, fmiInteger* value){

    UNUSED(c);
    UNUSED(s);
    UNUSED(value);

    logError((ModelInstance*)c, "Not implemented.");

    return fmiError;
}

fmiStatus fmiGetBooleanStatus(fmiComponent c, const fmiStatusKind s, fmiBoolean* value){

    UNUSED(c);
    UNUSED(s);
    UNUSED(value);

    logError((ModelInstance*)c, "Not implemented.");

    return fmiError;
}

fmiStatus fmiGetStringStatus(fmiComponent c, const fmiStatusKind s, fmiString*  value){

    UNUSED(c);
    UNUSED(s);
    UNUSED(value);

    logError((ModelInstance*)c, "Not implemented.");

    return fmiError;
}

#else
// ---------------------------------------------------------------------------
// FMI functions: only for Model Exchange 1.0
// ---------------------------------------------------------------------------

const char* fmiGetModelTypesPlatform() {
    return fmiModelTypesPlatform;
}

fmiComponent fmiInstantiateModel(fmiString instanceName, fmiString GUID,  fmiCallbackFunctions functions, fmiBoolean loggingOn) {

    if (!functions.logger) {
        return NULL;
    }

    return createModelInstance(
        (loggerType)functions.logger,
        NULL,
        NULL,
        instanceName,
        GUID,
        NULL,
        loggingOn,
        ModelExchange);
}

fmiStatus fmiInitialize(fmiComponent c, fmiBoolean toleranceControlled, fmiReal relativeTolerance, fmiEventInfo* eventInfo) {

    UNUSED(toleranceControlled);
    UNUSED(relativeTolerance);

    ModelInstance *instance = (ModelInstance *)c;

    fmiStatus status = init(c);

#ifdef EVENT_UPDATE
    eventUpdate(instance);
#endif

    eventInfo->iterationConverged          = instance->newDiscreteStatesNeeded;
    eventInfo->stateValueReferencesChanged = fmiFalse;
    eventInfo->stateValuesChanged          = instance->valuesOfContinuousStatesChanged;
    eventInfo->terminateSimulation         = instance->terminateSimulation;
    eventInfo->upcomingTimeEvent           = instance->nextEventTimeDefined;
    eventInfo->nextEventTime               = instance->nextEventTime;

    return status;
}

fmiStatus fmiSetTime(fmiComponent c, fmiReal time) {

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiSetTime", Instantiated|Initialized))
         return fmiError;

    instance->time = time;

    return fmiOK;
}

fmiStatus fmiSetContinuousStates(fmiComponent c, const fmiReal x[], size_t nx) {

    ModelInstance* instance = (ModelInstance *)c;

#ifdef HAS_CONTINUOUS_STATES
    if (invalidState(instance, "fmiSetContinuousStates", Initialized))
         return fmiError;

    if (invalidNumber(instance, "fmiSetContinuousStates", "nx", nx, getNumberOfContinuousStates(instance)))
        return fmiError;

    if (nullPointer(instance, "fmiSetContinuousStates", "x[]", x))
         return fmiError;

    setContinuousStates(instance, x, nx);
#else
    UNUSED(x);

    if (invalidNumber(instance, "fmiSetContinuousStates", "nx", nx, 0))
        return fmiError;
#endif

    return fmiOK;
}

fmiStatus fmiEventUpdate(fmiComponent c, fmiBoolean intermediateResults, fmiEventInfo* eventInfo) {

    UNUSED(intermediateResults);

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiEventUpdate", Initialized))
        return fmiError;

    if (nullPointer(instance, "fmiEventUpdate", "eventInfo", eventInfo))
         return fmiError;

#ifdef EVENT_UPDATE
    eventUpdate(instance);
#endif

    // copy internal eventInfo of component to output eventInfo
    eventInfo->iterationConverged          = fmiTrue;
    eventInfo->stateValueReferencesChanged = fmiFalse;
    eventInfo->stateValuesChanged          = instance->valuesOfContinuousStatesChanged;
    eventInfo->terminateSimulation         = instance->terminateSimulation;
    eventInfo->upcomingTimeEvent           = instance->nextEventTimeDefined;
    eventInfo->nextEventTime               = instance->nextEventTime;

    return fmiOK;
}

fmiStatus fmiCompletedIntegratorStep(fmiComponent c, fmiBoolean* callEventUpdate) {

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiCompletedIntegratorStep", Initialized))
         return fmiError;

    if (nullPointer(instance, "fmiCompletedIntegratorStep", "callEventUpdate", callEventUpdate))
         return fmiError;

    return fmiOK;
}

fmiStatus fmiGetStateValueReferences(fmiComponent c, fmiValueReference vrx[], size_t nx) {

    UNUSED(vrx);
    UNUSED(nx);

    logError((ModelInstance*)c, "Not implemented.");

    return fmiError;
}

fmiStatus fmiGetContinuousStates(fmiComponent c, fmiReal states[], size_t nx){

    ModelInstance* instance = (ModelInstance *)c;

#ifdef HAS_CONTINUOUS_STATES
    if (invalidState(instance, "fmiGetContinuousStates", not_modelError))
        return fmiError;

    if (invalidNumber(instance, "fmiGetContinuousStates", "nx", nx, getNumberOfContinuousStates(instance)))
        return fmiError;

    if (nullPointer(instance, "fmiGetContinuousStates", "states[]", states))
         return fmiError;

    getContinuousStates(instance, states, nx);
#else
    UNUSED(states);

    if (invalidNumber(instance, "fmiGetContinuousStates", "nx", nx, 0))
        return fmiError;
#endif

    return fmiOK;
}

fmiStatus fmiGetNominalContinuousStates(fmiComponent c, fmiReal x_nominal[], size_t nx) {

    ModelInstance* instance = (ModelInstance *)c;

#ifdef HAS_CONTINUOUS_STATES
    if (invalidState(instance, "fmiGetNominalContinuousStates", not_modelError))
        return fmiError;

    if (invalidNumber(instance, "fmiGetNominalContinuousStates", "nx", nx, getNumberOfContinuousStates(instance)))
        return fmiError;

    if (nullPointer(instance, "fmiGetNominalContinuousStates", "x_nominal[]", x_nominal))
         return fmiError;

    for (size_t i = 0; i < nx; i++) {
        x_nominal[i] = 1;
    }
#else
    UNUSED(x_nominal);

    if (invalidNumber(instance, "fmiGetNominalContinuousStates", "nx", nx, 0))
        return fmiError;
#endif

    return fmiOK;
}

fmiStatus fmiGetDerivatives(fmiComponent c, fmiReal derivatives[], size_t nx) {

    ModelInstance* instance = (ModelInstance *)c;

#ifdef HAS_CONTINUOUS_STATES
    if (invalidState(instance, "fmiGetDerivatives", not_modelError))
         return fmiError;

    if (invalidNumber(instance, "fmiGetDerivatives", "nx", nx, getNumberOfContinuousStates(instance)))
        return fmiError;

    if (nullPointer(instance, "fmiGetDerivatives", "derivatives[]", derivatives))
         return fmiError;

    getDerivatives(instance, derivatives, nx);
#else
    UNUSED(derivatives);

    if (invalidNumber(instance, "fmiGetDerivatives", "nx", nx, 0))
        return fmiError;
#endif

    return fmiOK;
}

fmiStatus fmiGetEventIndicators(fmiComponent c, fmiReal eventIndicators[], size_t ni) {

    ModelInstance* instance = (ModelInstance *)c;

#ifdef HAS_EVENT_INDICATORS
    if (invalidState(instance, "fmiGetEventIndicators", not_modelError))
        return fmiError;

    if (invalidNumber(instance, "fmiGetEventIndicators", "ni", ni, getNumberOfEventIndicators(instance)))
        return fmiError;

    getEventIndicators(instance, eventIndicators, ni);
#else
    UNUSED(eventIndicators);

    if (invalidNumber(instance, "fmiGetEventIndicators", "ni", ni, 0))
        return fmiError;
#endif

    return fmiOK;
}

fmiStatus fmiTerminate(fmiComponent c){
    return terminate("fmiTerminate", c);
}

void fmiFreeModelInstance(fmiComponent c) {
    freeModelInstance((ModelInstance*)c);
}

#endif // Model Exchange 1.0
