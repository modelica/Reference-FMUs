/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#if FMI_VERSION != 1
#error FMI_VERSION must be 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "config.h"
#include "model.h"
#include "cosimulation.h"


#ifdef FMI_COSIMULATION
#include "fmiFunctions.h"
#else
#include "fmiModelFunctions.h"
#endif

#define FMI_STATUS fmiStatus

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

#define ASSERT_STATE(F, A) \
    if (!c) return fmiError; \
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
    // TODO
    return fmiOK;
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
    // TODO
    return fmiOK;
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
        CoSimulation,
        false);
}

fmiStatus fmiInitializeSlave(fmiComponent c, fmiReal tStart, fmiBoolean StopTimeDefined, fmiReal tStop) {
    return init(c);
}

fmiStatus fmiTerminateSlave(fmiComponent c) {
    return terminate("fmiTerminateSlave", c);
}

fmiStatus fmiResetSlave(fmiComponent c) {
    ModelInstance* instance = (ModelInstance *)c;
    if (invalidState(instance, "fmiResetSlave", Initialized))
         return fmiError;
    instance->state = Instantiated;
    setStartValues(instance); // to be implemented by the includer of this file
    return fmiOK;
}

void fmiFreeSlaveInstance(fmiComponent c) {
    ModelInstance *instance = (ModelInstance *)c;
    freeModelInstance(instance);
}

fmiStatus fmiSetRealInputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr,
    const fmiInteger order[], const fmiReal value[]) {

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiSetRealInputDerivatives", Initialized))
         return fmiError;

    logError(instance, "fmiSetRealInputDerivatives: This model cannot interpolate inputs: canInterpolateInputs=\"fmiFalse\"");

    return fmiError;
}

fmiStatus fmiGetRealOutputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger order[], fmiReal value[]) {

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
    ModelInstance* instance = (ModelInstance *)c;

    bool eventEncountered, terminateSimulation, earlyReturn;
    double lastSuccessfulTime;

    return (fmiStatus)doStep(instance, currentCommunicationPoint, currentCommunicationPoint + communicationStepSize, &eventEncountered, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);
}

static fmiStatus getStatus(char* fname, fmiComponent c, const fmiStatusKind s) {
//    const char* statusKind[3] = {"fmiDoStepStatus","fmiPendingStatus","fmiLastSuccessfulTime"};
//    ModelInstance* instance = (ModelInstance *)c;
//    fmiCallbackLogger log = instance->functions.logger;
//    if (invalidState(instance, fname, Instantiated|Initialized))
//         return fmiError;
//    if (instance->loggingOn) log(c, instance->instanceName, fmiOK, "log", "$s: fmiStatusKind = %s", fname, statusKind[s]);
//    switch(s) {
//        case fmiDoStepStatus:  log(c, instance->instanceName, fmiError, "error",
//           "%s: Can be called with fmiDoStepStatus when fmiDoStep returned fmiPending."
//           " This is not the case.", fname);
//           break;
//        case fmiPendingStatus:  log(c, instance->instanceName, fmiError, "error",
//           "%s: Can be called with fmiPendingStatus when fmiDoStep returned fmiPending."
//           " This is not the case.", fname);
//           break;
//        case fmiLastSuccessfulTime:  log(c, instance->instanceName, fmiError, "error",
//           "%s: Can be called with fmiLastSuccessfulTime when fmiDoStep returned fmiDiscard."
//           " This is not the case.", fname);
//           break;
//    }
    return fmiError;
}

fmiStatus fmiGetStatus(fmiComponent c, const fmiStatusKind s, fmiStatus* value) {
    return getStatus("fmiGetStatus", c, s);
}

fmiStatus fmiGetRealStatus(fmiComponent c, const fmiStatusKind s, fmiReal* value){
    return getStatus("fmiGetRealStatus", c, s);
}

fmiStatus fmiGetIntegerStatus(fmiComponent c, const fmiStatusKind s, fmiInteger* value){
    return getStatus("fmiGetIntegerStatus", c, s);
}

fmiStatus fmiGetBooleanStatus(fmiComponent c, const fmiStatusKind s, fmiBoolean* value){
    return getStatus("fmiGetBooleanStatus", c, s);
}

fmiStatus fmiGetStringStatus(fmiComponent c, const fmiStatusKind s, fmiString*  value){
    return getStatus("fmiGetStringStatus", c, s);
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
        ModelExchange,
        false);
}

fmiStatus fmiInitialize(fmiComponent c, fmiBoolean toleranceControlled, fmiReal relativeTolerance, fmiEventInfo* eventInfo) {

    ModelInstance *instance = (ModelInstance *)c;

    fmiStatus status = init(c);

    eventUpdate(instance);

    eventInfo->iterationConverged          = instance->newDiscreteStatesNeeded ? fmiFalse : fmiTrue;
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

    if (invalidState(instance, "fmiSetContinuousStates", Initialized))
         return fmiError;

    if (invalidNumber(instance, "fmiSetContinuousStates", "nx", nx, NX))
        return fmiError;

    if (nullPointer(instance, "fmiSetContinuousStates", "x[]", x))
         return fmiError;

    setContinuousStates(instance, x, nx);

    return fmiOK;
}

fmiStatus fmiEventUpdate(fmiComponent c, fmiBoolean intermediateResults, fmiEventInfo* eventInfo) {

    ModelInstance* instance = (ModelInstance *)c;

    int timeEvent = 0;

    if (invalidState(instance, "fmiEventUpdate", Initialized))
        return fmiError;

    if (nullPointer(instance, "fmiEventUpdate", "eventInfo", eventInfo))
         return fmiError;

    instance->newDiscreteStatesNeeded           = false;
    instance->terminateSimulation               = false;
    instance->nominalsOfContinuousStatesChanged = false;
    instance->valuesOfContinuousStatesChanged   = false;

    if (instance->nextEventTimeDefined && instance->nextEventTime <= instance->time) {
        timeEvent = 1;
    }

    eventUpdate(instance);

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
//    int i;
//    ModelInstance* instance = (ModelInstance *)c;
//    if (invalidState(instance, "fmiGetStateValueReferences", not_modelError))
//        return fmiError;
//    if (invalidNumber(instance, "fmiGetStateValueReferences", "nx", nx, NX))
//        return fmiError;
//    if (nullPointer(instance, "fmiGetStateValueReferences", "vrx[]", vrx))
//         return fmiError;
//#if NX>0
//    for (i=0; i<nx; i++) {
//        vrx[i] = vrStates[i];
//        if (instance->loggingOn) instance->functions.logger(c, instance->instanceName, fmiOK, "log",
//            "fmiGetStateValueReferences: vrx[%d] = %d", i, vrx[i]);
//    }
//#endif
    return fmiOK;
}

fmiStatus fmiGetContinuousStates(fmiComponent c, fmiReal states[], size_t nx){

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiGetContinuousStates", not_modelError))
        return fmiError;

    if (invalidNumber(instance, "fmiGetContinuousStates", "nx", nx, NX))
        return fmiError;

    if (nullPointer(instance, "fmiGetContinuousStates", "states[]", states))
         return fmiError;

    getContinuousStates(instance, states, nx);

    return fmiOK;
}

fmiStatus fmiGetNominalContinuousStates(fmiComponent c, fmiReal x_nominal[], size_t nx) {
//    int i;
//    ModelInstance* instance = (ModelInstance *)c;
//    if (invalidState(instance, "fmiGetNominalContinuousStates", not_modelError))
//        return fmiError;
//    if (invalidNumber(instance, "fmiGetNominalContinuousStates", "nx", nx, NX))
//        return fmiError;
//    if (nullPointer(instance, "fmiGetNominalContinuousStates", "x_nominal[]", x_nominal))
//         return fmiError;
//    if (instance->loggingOn) instance->functions.logger(c, instance->instanceName, fmiOK, "log",
//        "fmiGetNominalContinuousStates: x_nominal[0..%d] = 1.0", nx-1);
//    for (i=0; i<nx; i++)
//        x_nominal[i] = 1;
    return fmiOK;
}

fmiStatus fmiGetDerivatives(fmiComponent c, fmiReal derivatives[], size_t nx) {

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiGetDerivatives", not_modelError))
         return fmiError;

    if (invalidNumber(instance, "fmiGetDerivatives", "nx", nx, NX))
        return fmiError;

    if (nullPointer(instance, "fmiGetDerivatives", "derivatives[]", derivatives))
         return fmiError;

    getDerivatives(instance, derivatives, nx);

    return fmiOK;
}

fmiStatus fmiGetEventIndicators(fmiComponent c, fmiReal eventIndicators[], size_t ni) {

    ModelInstance* instance = (ModelInstance *)c;

    if (invalidState(instance, "fmiGetEventIndicators", not_modelError))
        return fmiError;

    if (invalidNumber(instance, "fmiGetEventIndicators", "ni", ni, NZ))
        return fmiError;

    getEventIndicators(instance, eventIndicators, ni);

    return fmiOK;
}

fmiStatus fmiTerminate(fmiComponent c){
    return terminate("fmiTerminate", c);
}

void fmiFreeModelInstance(fmiComponent c) {
    ModelInstance *instance = (ModelInstance *)c;
    freeModelInstance(instance);
}

#endif // Model Exchange 1.0
