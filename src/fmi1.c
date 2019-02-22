/****************************************************************
 *  Copyright (c) Dassault Systemes. All rights reserved.       *
 *  This file is part of the Test-FMUs. See LICENSE.txt in the  *
 *  project root for license information.                       *
 ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "config.h"
#include "model.h"
#include "solver.h"
#include "slave.h"


#ifdef FMI_COSIMULATION
#include "fmiFunctions.h"
#else
#include "fmiModelFunctions.h"
#endif

//// array of value references of states
//#if NUMBER_OF_STATES>0
//fmiValueReference vrStates[NUMBER_OF_STATES] = STATES;
//#endif

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

void logError(ModelInstance *comp, const char *message, ...) {

    va_list args;
    size_t len = 0;
    char *buf = "";

    va_start(args, message);
    len = vsnprintf(buf, len, message, args);
    va_end(args);

    buf = malloc(len + 1);

    va_start(args, message);
    len = vsnprintf(buf, len + 1, message, args);
    va_end(args);

    comp->logger(comp->componentEnvironment, comp->instanceName, fmiError, "logError", buf);

    free(buf);
}

static fmiBoolean invalidNumber(ModelInstance* comp, const char* f, const char* arg, int n, int nExpected){
    if (n != nExpected) {
        comp->state = modelError;
        comp->logger(comp, comp->instanceName, fmiError, "error",
                "%s: Invalid argument %s = %d. Expected %d.", f, arg, n, nExpected);
        return fmiTrue;
    }
    return fmiFalse;
}

static fmiBoolean invalidState(ModelInstance* comp, const char* f, int statesExpected){
    if (!comp)
        return fmiTrue;
    if (!(comp->state & statesExpected)) {
        comp->state = modelError;
        comp->logger(comp, comp->instanceName, fmiError, "error",
                "%s: Illegal call sequence.", f);
        return fmiTrue;
    }
    return fmiFalse;
}

static fmiBoolean nullPointer(ModelInstance* comp, const char* f, const char* arg, const void* p){
    if (!p) {
        comp->state = modelError;
        comp->logger(comp, comp->instanceName, fmiError, "error",
                "%s: Invalid argument %s = NULL.", f, arg);
        return fmiTrue;
    }
    return fmiFalse;
}

// ---------------------------------------------------------------------------
// Private helpers used below to implement functions
// ---------------------------------------------------------------------------

fmiStatus setString(fmiComponent comp, fmiValueReference vr, fmiString value){
    return fmiSetString(comp, &vr, 1, &value);
}

// fname is fmiInstantiateModel or fmiInstantiateSlave
static fmiComponent instantiateModel(char* fname, fmiString instanceName, fmiString GUID,
        fmiString fmuLocation, fmiCallbackFunctions functions, fmiBoolean loggingOn) {

    ModelInstance* comp;

    if (!functions.logger)
        return NULL; // we cannot even log this problem

    if (!instanceName || strlen(instanceName) == 0) {
        functions.logger(NULL, "?", fmiError, "error",
                "%s: Missing instance name.", fname);
        return NULL;
    }

    if (!GUID || strlen(GUID) == 0) {
        functions.logger(NULL, instanceName, fmiError, "error",
                "%s: Missing GUID.", fname);
        return NULL;
    }

#if CO_SIMULATION
    if (!fmuLocation || strlen(fmuLocation) == 0) {
        functions.logger(NULL, instanceName, fmiError, "error",
                         "%s: Missing fmuLocation.", fname);
        return NULL;
    }
#endif

    if (!functions.allocateMemory || !functions.freeMemory){
        functions.logger(NULL, instanceName, fmiError, "error",
                "%s: Missing callback function.", fname);
        return NULL;
    }

    if (strcmp(GUID, MODEL_GUID)) {
        functions.logger(NULL, instanceName, fmiError, "error",
                "%s: Wrong GUID %s. Expected %s.", fname, GUID, MODEL_GUID);
        return NULL;
    }

    comp = (ModelInstance *)functions.allocateMemory(1, sizeof(ModelInstance));

    if (comp) {
        comp->instanceName = (char *)functions.allocateMemory(1 + strlen(instanceName), sizeof(char));
        comp->GUID = (char *)functions.allocateMemory(1 + strlen(GUID), sizeof(char));
#ifdef FMI_COSIMULATION
        comp->resourceLocation = (char *)functions.allocateMemory(1 + strlen(fmuLocation), sizeof(char));
#else
        comp->resourceLocation = NULL;
#endif
        comp->modelData = (ModelData *)functions.allocateMemory(1, sizeof(ModelData));
    }

    if (!comp || !comp->instanceName || !comp->GUID) {
        functions.logger(NULL, instanceName, fmiError, "error",
                "%s: Out of memory.", fname);
        return NULL;
    }

    if (loggingOn) functions.logger(NULL, instanceName, fmiOK, "log",
            "%s: GUID=%s", fname, GUID);

    strcpy((char *)comp->instanceName, (char *)instanceName);
    strcpy((char *)comp->GUID, (char *)GUID);
#ifdef FMI_COSIMULATION
    strcpy((char *)comp->resourceLocation, (char *)fmuLocation);
#endif
    comp->logger = functions.logger;
    comp->allocateMemory = functions.allocateMemory;
    comp->freeMemory = functions.freeMemory;
    comp->loggingOn = loggingOn;
    comp->state = modelInstantiated;

    setStartValues(comp); // to be implemented by the includer of this file

    comp->solverData = solver_create(comp);

    return comp;
}

// fname is fmiInitialize or fmiInitializeSlave
static fmiStatus init(fmiComponent c) {
    ModelInstance* comp = (ModelInstance *)c;
    comp->state = modelInitialized;
    calculateValues(comp);
    return fmiOK;
}

// fname is fmiTerminate or fmiTerminateSlave
static fmiStatus terminate(char* fname, fmiComponent c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, fname, modelInitialized))
         return fmiError;
    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log", fname);
    comp->state = modelTerminated;
    return fmiOK;
}

// fname is freeModelInstance of freeSlaveInstance
void freeInstance(char* fname, fmiComponent c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (!comp) return;
    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log", fname);
    if (comp->instanceName) comp->freeMemory((void *)comp->instanceName);
    if (comp->GUID) comp->freeMemory((void *)comp->GUID);
    comp->freeMemory(comp);
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
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiSetDebugLogging", not_modelError))
         return fmiError;
    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log",
            "fmiSetDebugLogging: loggingOn=%d", loggingOn);
    comp->loggingOn = loggingOn;
    return fmiOK;
}

fmiStatus fmiSetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiReal value[]) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiSetReal", modelInstantiated|modelInitialized))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiSetReal", "vr[]", vr))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiSetReal", "value[]", value))
         return fmiError;

    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log",
            "fmiSetReal: nvr = %d", nvr);
    
#ifdef SET_FLOAT64
    SET_VARIABLES(Float64)
#else
    return fmiError;  // not implemented
#endif
}

fmiStatus fmiSetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiInteger value[]) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiSetInteger", modelInstantiated|modelInitialized))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiSetInteger", "vr[]", vr))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiSetInteger", "value[]", value))
         return fmiError;

    if (comp->loggingOn)
        comp->logger(c, comp->instanceName, fmiOK, "log", "fmiSetInteger: nvr = %d",  nvr);

#ifdef SET_INT32
    SET_VARIABLES(Int32)
#else
    return fmiError;  // not implemented
#endif
}

fmiStatus fmiSetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiBoolean value[]){

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiSetBoolean", modelInstantiated|modelInitialized))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiSetBoolean", "vr[]", vr))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiSetBoolean", "value[]", value))
         return fmiError;

    if (comp->loggingOn)
        comp->logger(c, comp->instanceName, fmiOK, "log", "fmiSetBoolean: nvr = %d",  nvr);

    SET_BOOLEAN_VARIABLES
}

fmiStatus fmiSetString(fmiComponent c, const fmiValueReference vr[], size_t nvr, const fmiString value[]){
//    int i;
//    ModelInstance* comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmiSetString", modelInstantiated|modelInitialized))
//         return fmiError;
//    if (nvr>0 && nullPointer(comp, "fmiSetString", "vr[]", vr))
//         return fmiError;
//    if (nvr>0 && nullPointer(comp, "fmiSetString", "value[]", value))
//         return fmiError;
//    if (comp->loggingOn)
//        comp->functions.logger(c, comp->instanceName, fmiOK, "log", "fmiSetString: nvr = %d",  nvr);
//    for (i=0; i<nvr; i++) {
//        char *string = (char *)comp->s[vr[i]];
//        if (vrOutOfRange(comp, "fmiSetString", vr[i], NUMBER_OF_STRINGS))
//            return fmiError;
//        if (comp->loggingOn) comp->functions.logger(c, comp->instanceName, fmiOK, "log",
//            "fmiSetString: #s%d# = '%s'", vr[i], value[i]);
//        if (value[i] == NULL) {
//            if (string) comp->functions.freeMemory(string);
//            comp->s[vr[i]] = NULL;
//            comp->functions.logger(comp, comp->instanceName, fmiWarning, "warning",
//                            "fmiSetString: string argument value[%d] = NULL.", i);
//        } else {
//            if (string==NULL || strlen(string) < strlen(value[i])) {
//                if (string) comp->functions.freeMemory(string);
//                comp->s[vr[i]] = (char *)comp->functions.allocateMemory(1+strlen(value[i]), sizeof(char));
//                if (!comp->s[vr[i]]) {
//                    comp->state = modelError;
//                    comp->functions.logger(NULL, comp->instanceName, fmiError, "error", "fmiSetString: Out of memory.");
//                    return fmiError;
//                }
//            }
//            strcpy((char *)comp->s[vr[i]], (char *)value[i]);
//        }
//    }
    return fmiOK;
}

fmiStatus fmiGetReal(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiReal value[]) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiGetReal", not_modelError))
        return fmiError;

    if (nvr > 0 && nullPointer(comp, "fmiGetReal", "vr[]", vr))
         return fmiError;

    if (nvr > 0 && nullPointer(comp, "fmiGetReal", "value[]", value))
         return fmiError;
    
    GET_VARIABLES(Float64)
}

fmiStatus fmiGetInteger(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiInteger value[]) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiGetInteger", not_modelError))
        return fmiError;

    if (nvr > 0 && nullPointer(comp, "fmiGetInteger", "vr[]", vr))
         return fmiError;

    if (nvr > 0 && nullPointer(comp, "fmiGetInteger", "value[]", value))
         return fmiError;

    GET_VARIABLES(Int32)
}

fmiStatus fmiGetBoolean(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiBoolean value[]) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiGetBoolean", not_modelError))
        return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiGetBoolean", "vr[]", vr))
         return fmiError;

    if (nvr>0 && nullPointer(comp, "fmiGetBoolean", "value[]", value))
         return fmiError;

    GET_BOOLEAN_VARIABLES
}

fmiStatus fmiGetString(fmiComponent c, const fmiValueReference vr[], size_t nvr, fmiString  value[]) {
//    int i;
//    ModelInstance* comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmiGetString", not_modelError))
//        return fmiError;
//    if (nvr>0 && nullPointer(comp, "fmiGetString", "vr[]", vr))
//         return fmiError;
//    if (nvr>0 && nullPointer(comp, "fmiGetString", "value[]", value))
//         return fmiError;
//    for (i=0; i<nvr; i++) {
//        if (vrOutOfRange(comp, "fmiGetString", vr[i], NUMBER_OF_STRINGS))
//           return fmiError;
//        value[i] = comp->s[vr[i]];
//        if (comp->loggingOn) comp->functions.logger(c, comp->instanceName, fmiOK, "log",
//                "fmiGetString: #s%u# = '%s'", vr[i], value[i]);
//    }
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
    // ignoring arguments: mimeType, timeout, visible, interactive
    return instantiateModel("fmiInstantiateSlave", instanceName, GUID, fmuLocation, functions, loggingOn);
}

fmiStatus fmiInitializeSlave(fmiComponent c, fmiReal tStart, fmiBoolean StopTimeDefined, fmiReal tStop) {
    init(c);
    return fmiOK;
}

fmiStatus fmiTerminateSlave(fmiComponent c) {
    return terminate("fmiTerminateSlave", c);
}

fmiStatus fmiResetSlave(fmiComponent c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmiResetSlave", modelInitialized))
         return fmiError;
    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log", "fmiResetSlave");
    comp->state = modelInstantiated;
    setStartValues(comp); // to be implemented by the includer of this file
    return fmiOK;
}

void fmiFreeSlaveInstance(fmiComponent c) {
//    ModelInstance* comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmiFreeSlaveInstance", modelTerminated))
//         return;
//    freeInstance("fmiFreeSlaveInstance", c);
}

fmiStatus fmiSetRealInputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr,
    const fmiInteger order[], const fmiReal value[]) {
    ModelInstance* comp = (ModelInstance *)c;
    fmiCallbackLogger log = comp->logger;
    if (invalidState(comp, "fmiSetRealInputDerivatives", modelInitialized))
         return fmiError;
    if (comp->loggingOn) log(c, comp->instanceName, fmiOK, "log", "fmiSetRealInputDerivatives: nvr= %d", nvr);
    log(c, comp->instanceName, fmiError, "warning", "fmiSetRealInputDerivatives: ignoring function call."
      " This model cannot interpolate inputs: canInterpolateInputs=\"fmiFalse\"");
    return fmiWarning;
}

fmiStatus fmiGetRealOutputDerivatives(fmiComponent c, const fmiValueReference vr[], size_t nvr,
    const fmiInteger order[], fmiReal value[]) {
    int i;
    ModelInstance* comp = (ModelInstance *)c;
    fmiCallbackLogger log = comp->logger;
    if (invalidState(comp, "fmiGetRealOutputDerivatives", modelInitialized))
         return fmiError;
    if (comp->loggingOn) log(c, comp->instanceName, fmiOK, "log", "fmiGetRealOutputDerivatives: nvr= %d", nvr);
    log(c, comp->instanceName, fmiError, "warning", "fmiGetRealOutputDerivatives: ignoring function call."
      " This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"");
    for (i=0; i<nvr; i++) value[i] = 0;
    return fmiWarning;
}

fmiStatus fmiCancelStep(fmiComponent c) {
    ModelInstance* comp = (ModelInstance *)c;
    fmiCallbackLogger log = comp->logger;
    if (invalidState(comp, "fmiCancelStep", modelInitialized))
         return fmiError;
    if (comp->loggingOn) log(c, comp->instanceName, fmiOK, "log", "fmiCancelStep");
    log(c, comp->instanceName, fmiError, "error",
        "fmiCancelStep: Can be called when fmiDoStep returned fmiPending."
        " This is not the case.");
    return fmiError;
}

fmiStatus fmiDoStep(fmiComponent c, fmiReal currentCommunicationPoint, fmiReal communicationStepSize, fmiBoolean newStep) {
    ModelInstance* comp = (ModelInstance *)c;
    return doStep(comp, currentCommunicationPoint, currentCommunicationPoint + communicationStepSize);
}

static fmiStatus getStatus(char* fname, fmiComponent c, const fmiStatusKind s) {
//    const char* statusKind[3] = {"fmiDoStepStatus","fmiPendingStatus","fmiLastSuccessfulTime"};
//    ModelInstance* comp = (ModelInstance *)c;
//    fmiCallbackLogger log = comp->functions.logger;
//    if (invalidState(comp, fname, modelInstantiated|modelInitialized))
//         return fmiError;
//    if (comp->loggingOn) log(c, comp->instanceName, fmiOK, "log", "$s: fmiStatusKind = %s", fname, statusKind[s]);
//    switch(s) {
//        case fmiDoStepStatus:  log(c, comp->instanceName, fmiError, "error",
//           "%s: Can be called with fmiDoStepStatus when fmiDoStep returned fmiPending."
//           " This is not the case.", fname);
//           break;
//        case fmiPendingStatus:  log(c, comp->instanceName, fmiError, "error",
//           "%s: Can be called with fmiPendingStatus when fmiDoStep returned fmiPending."
//           " This is not the case.", fname);
//           break;
//        case fmiLastSuccessfulTime:  log(c, comp->instanceName, fmiError, "error",
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
    return instantiateModel("fmiInstantiateModel", instanceName, GUID, NULL, functions, loggingOn);
}

fmiStatus fmiInitialize(fmiComponent c, fmiBoolean toleranceControlled, fmiReal relativeTolerance, fmiEventInfo* eventInfo) {
    return init(c);
}

fmiStatus fmiSetTime(fmiComponent c, fmiReal time) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiSetTime", modelInstantiated|modelInitialized))
         return fmiError;

    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log", "fmiSetTime: time=%.16g", time);

    comp->time = time;

    return fmiOK;
}

fmiStatus fmiSetContinuousStates(fmiComponent c, const fmiReal x[], size_t nx) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiSetContinuousStates", modelInitialized))
         return fmiError;

    if (invalidNumber(comp, "fmiSetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmiError;

    if (nullPointer(comp, "fmiSetContinuousStates", "x[]", x))
         return fmiError;

    setContinuousStates(comp, x, nx);

    return fmiOK;
}

fmiStatus fmiEventUpdate(fmiComponent c, fmiBoolean intermediateResults, fmiEventInfo* eventInfo) {

    ModelInstance* comp = (ModelInstance *)c;

    int timeEvent = 0;

    if (invalidState(comp, "fmiEventUpdate", modelInitialized))
        return fmiError;

    if (nullPointer(comp, "fmiEventUpdate", "eventInfo", eventInfo))
         return fmiError;

    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log",
        "fmiEventUpdate: intermediateResults = %d", intermediateResults);

    comp->newDiscreteStatesNeeded           = false;
    comp->terminateSimulation               = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->valuesOfContinuousStatesChanged   = false;

    if (comp->nextEventTimeDefined && comp->nextEventTime <= comp->time) {
        timeEvent = 1;
    }

    eventUpdate(comp);

    // copy internal eventInfo of component to output eventInfo
    eventInfo->iterationConverged          = fmiTrue;
    eventInfo->stateValueReferencesChanged = fmiFalse;
    eventInfo->stateValuesChanged          = comp->valuesOfContinuousStatesChanged;
    eventInfo->terminateSimulation         = comp->terminateSimulation;
    eventInfo->upcomingTimeEvent           = comp->nextEventTimeDefined;
    eventInfo->nextEventTime               = comp->nextEventTime;

    return fmiOK;
}

fmiStatus fmiCompletedIntegratorStep(fmiComponent c, fmiBoolean* callEventUpdate) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiCompletedIntegratorStep", modelInitialized))
         return fmiError;

    if (nullPointer(comp, "fmiCompletedIntegratorStep", "callEventUpdate", callEventUpdate))
         return fmiError;

    if (comp->loggingOn) comp->logger(c, comp->instanceName, fmiOK, "log", "fmiCompletedIntegratorStep");

    return fmiOK;
}

fmiStatus fmiGetStateValueReferences(fmiComponent c, fmiValueReference vrx[], size_t nx) {
//    int i;
//    ModelInstance* comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmiGetStateValueReferences", not_modelError))
//        return fmiError;
//    if (invalidNumber(comp, "fmiGetStateValueReferences", "nx", nx, NUMBER_OF_STATES))
//        return fmiError;
//    if (nullPointer(comp, "fmiGetStateValueReferences", "vrx[]", vrx))
//         return fmiError;
//#if NUMBER_OF_STATES>0
//    for (i=0; i<nx; i++) {
//        vrx[i] = vrStates[i];
//        if (comp->loggingOn) comp->functions.logger(c, comp->instanceName, fmiOK, "log",
//            "fmiGetStateValueReferences: vrx[%d] = %d", i, vrx[i]);
//    }
//#endif
    return fmiOK;
}

fmiStatus fmiGetContinuousStates(fmiComponent c, fmiReal states[], size_t nx){

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiGetContinuousStates", not_modelError))
        return fmiError;

    if (invalidNumber(comp, "fmiGetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmiError;

    if (nullPointer(comp, "fmiGetContinuousStates", "states[]", states))
         return fmiError;

    getContinuousStates(comp, states, nx);

    return fmiOK;
}

fmiStatus fmiGetNominalContinuousStates(fmiComponent c, fmiReal x_nominal[], size_t nx) {
//    int i;
//    ModelInstance* comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmiGetNominalContinuousStates", not_modelError))
//        return fmiError;
//    if (invalidNumber(comp, "fmiGetNominalContinuousStates", "nx", nx, NUMBER_OF_STATES))
//        return fmiError;
//    if (nullPointer(comp, "fmiGetNominalContinuousStates", "x_nominal[]", x_nominal))
//         return fmiError;
//    if (comp->loggingOn) comp->functions.logger(c, comp->instanceName, fmiOK, "log",
//        "fmiGetNominalContinuousStates: x_nominal[0..%d] = 1.0", nx-1);
//    for (i=0; i<nx; i++)
//        x_nominal[i] = 1;
    return fmiOK;
}

fmiStatus fmiGetDerivatives(fmiComponent c, fmiReal derivatives[], size_t nx) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiGetDerivatives", not_modelError))
         return fmiError;

    if (invalidNumber(comp, "fmiGetDerivatives", "nx", nx, NUMBER_OF_STATES))
        return fmiError;

    if (nullPointer(comp, "fmiGetDerivatives", "derivatives[]", derivatives))
         return fmiError;

    getDerivatives(comp, derivatives, nx);

    return fmiOK;
}

fmiStatus fmiGetEventIndicators(fmiComponent c, fmiReal eventIndicators[], size_t ni) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmiGetEventIndicators", not_modelError))
        return fmiError;

    if (invalidNumber(comp, "fmiGetEventIndicators", "ni", ni, NUMBER_OF_EVENT_INDICATORS))
        return fmiError;

    getEventIndicators(comp, eventIndicators, ni);

    return fmiOK;
}

fmiStatus fmiTerminate(fmiComponent c){
    return terminate("fmiTerminate", c);
}

void fmiFreeModelInstance(fmiComponent c) {
    freeInstance("fmiFreeModelInstance", c);
}

#endif // Model Exchange 1.0
