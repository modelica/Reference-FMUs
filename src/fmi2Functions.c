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
#include "slave.h"


// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#ifndef DISABLE_PREFIX
#define pasteA(a,b)     a ## b
#define pasteB(a,b)    pasteA(a,b)
#define FMI2_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi2Functions.h"

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Function calls allowed state masks for both Model-exchange and Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi2GetTypesPlatform        (modelStartAndEnd | modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepInProgress | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi2GetVersion              MASK_fmi2GetTypesPlatform
#define MASK_fmi2SetDebugLogging         (modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepInProgress | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi2Instantiate             (modelStartAndEnd)
#define MASK_fmi2FreeInstance            (modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi2SetupExperiment         modelInstantiated
#define MASK_fmi2EnterInitializationMode modelInstantiated
#define MASK_fmi2ExitInitializationMode  modelInitializationMode
#define MASK_fmi2Terminate               (modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed)
#define MASK_fmi2Reset                   MASK_fmi2FreeInstance
#define MASK_fmi2GetReal                 (modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi2GetInteger              MASK_fmi2GetReal
#define MASK_fmi2GetBoolean              MASK_fmi2GetReal
#define MASK_fmi2GetString               MASK_fmi2GetReal
#define MASK_fmi2SetReal                 (modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete)
#define MASK_fmi2SetInteger              (modelInstantiated | modelInitializationMode \
| modelEventMode \
| modelStepComplete)
#define MASK_fmi2SetBoolean              MASK_fmi2SetInteger
#define MASK_fmi2SetString               MASK_fmi2SetInteger
#define MASK_fmi2GetFMUstate             MASK_fmi2FreeInstance
#define MASK_fmi2SetFMUstate             MASK_fmi2FreeInstance
#define MASK_fmi2FreeFMUstate            MASK_fmi2FreeInstance
#define MASK_fmi2SerializedFMUstateSize  MASK_fmi2FreeInstance
#define MASK_fmi2SerializeFMUstate       MASK_fmi2FreeInstance
#define MASK_fmi2DeSerializeFMUstate     MASK_fmi2FreeInstance
#define MASK_fmi2GetDirectionalDerivative (modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Model-exchange
// ---------------------------------------------------------------------------
#define MASK_fmi2EnterEventMode          (modelEventMode | modelContinuousTimeMode)
#define MASK_fmi2NewDiscreteStates       modelEventMode
#define MASK_fmi2EnterContinuousTimeMode modelEventMode
#define MASK_fmi2CompletedIntegratorStep modelContinuousTimeMode
#define MASK_fmi2SetTime                 (modelEventMode | modelContinuousTimeMode)
#define MASK_fmi2SetContinuousStates     modelContinuousTimeMode
#define MASK_fmi2GetEventIndicators      (modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelTerminated | modelError)
#define MASK_fmi2GetContinuousStates     MASK_fmi2GetEventIndicators
#define MASK_fmi2GetDerivatives          (modelEventMode | modelContinuousTimeMode \
| modelTerminated | modelError)
#define MASK_fmi2GetNominalsOfContinuousStates ( modelInstantiated \
| modelEventMode | modelContinuousTimeMode \
| modelTerminated | modelError)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi2SetRealInputDerivatives (modelInstantiated | modelInitializationMode \
| modelStepComplete)
#define MASK_fmi2GetRealOutputDerivatives (modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi2DoStep                  modelStepComplete
#define MASK_fmi2CancelStep              modelStepInProgress
#define MASK_fmi2GetStatus               (modelStepComplete | modelStepInProgress | modelStepFailed \
| modelTerminated)
#define MASK_fmi2GetRealStatus           MASK_fmi2GetStatus
#define MASK_fmi2GetIntegerStatus        MASK_fmi2GetStatus
#define MASK_fmi2GetBooleanStatus        MASK_fmi2GetStatus
#define MASK_fmi2GetStringStatus         MASK_fmi2GetStatus

// ---------------------------------------------------------------------------
// Private helpers used below to validate function arguments
// ---------------------------------------------------------------------------

static fmi2Status unsupportedFunction(fmi2Component c, const char *fName, int statesExpected) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, fName, statesExpected))
        return fmi2Error;
	logError(comp, "%s: Function not implemented.", fName);
    return fmi2Error;
}

// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                            fmi2String fmuResourceLocation, const fmi2CallbackFunctions *functions,
                            fmi2Boolean visible, fmi2Boolean loggingOn) {

	return createModelInstance(
		(loggerType)functions->logger,
		(allocateMemoryType)functions->allocateMemory,
		(freeMemoryType)functions->freeMemory,
		functions->componentEnvironment,
		instanceName,
		fmuGUID,
		fmuResourceLocation,
		loggingOn,
		fmuType
	);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                            fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {

    // ignore arguments: stopTimeDefined, stopTime
    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetupExperiment", MASK_fmi2SetupExperiment))
        return fmi2Error;

    comp->time = startTime;

    return fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterInitializationMode", MASK_fmi2EnterInitializationMode))
        return fmi2Error;
    comp->state = modelInitializationMode;
    return fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2ExitInitializationMode", MASK_fmi2ExitInitializationMode))
        return fmi2Error;

    // if values were set and no fmi2GetXXX triggered update before,
    // ensure calculated values are updated now
    if (comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    if (comp->type == ModelExchange) {
        comp->state = modelEventMode;
        comp->isNewEventIteration = false;
    }
    else comp->state = modelStepComplete;
    return fmi2OK;
}

fmi2Status fmi2Terminate(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Terminate", MASK_fmi2Terminate))
        return fmi2Error;
    comp->state = modelTerminated;
    return fmi2OK;
}

fmi2Status fmi2Reset(fmi2Component c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2Reset", MASK_fmi2Reset))
        return fmi2Error;
    comp->state = modelInstantiated;
    setStartValues(comp); // to be implemented by the includer of this file
    comp->isDirtyValues = true; // because we just called setStartValues
    return fmi2OK;
}

void fmi2FreeInstance(fmi2Component c) {

    ModelInstance *comp = (ModelInstance *)c;

    if (!comp) return;

    if (invalidState(comp, "fmi2FreeInstance", MASK_fmi2FreeInstance))
        return;

	freeModelInstance(comp);
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmi2GetVersion() {
    return fmi2Version;
}

const char* fmi2GetTypesPlatform() {
    return fmi2TypesPlatform;
}

// ---------------------------------------------------------------------------
// FMI functions: logging control, setters and getters for Real, Integer,
// Boolean, String
// ---------------------------------------------------------------------------

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
    
    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetDebugLogging", MASK_fmi2SetDebugLogging)) return fmi2Error;

    return setDebugLogging(comp, loggingOn, nCategories, categories);
}

fmi2Status fmi2GetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetReal", MASK_fmi2GetReal))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "vr[]", vr))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2GetReal", "value[]", value))
        return fmi2Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_VARIABLES(Float64)
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetInteger", MASK_fmi2GetInteger))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "vr[]", vr))
            return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2GetInteger", "value[]", value))
            return fmi2Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_VARIABLES(Int32)
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetBoolean", MASK_fmi2GetBoolean))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "vr[]", vr))
            return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2GetBoolean", "value[]", value))
            return fmi2Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_BOOLEAN_VARIABLES
}

fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetString", MASK_fmi2GetString))
        return fmi2Error;

    if (nvr>0 && nullPointer(comp, "fmi2GetString", "vr[]", vr))
            return fmi2Error;

    if (nvr>0 && nullPointer(comp, "fmi2GetString", "value[]", value))
            return fmi2Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_VARIABLES(String)
}

fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetReal", MASK_fmi2SetReal))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "vr[]", vr))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2SetReal", "value[]", value))
        return fmi2Error;

    SET_VARIABLES(Float64)
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetInteger", MASK_fmi2SetInteger))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "vr[]", vr))
        return fmi2Error;

    if (nvr > 0 && nullPointer(comp, "fmi2SetInteger", "value[]", value))
        return fmi2Error;

    SET_VARIABLES(Int32)
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetBoolean", MASK_fmi2SetBoolean))
        return fmi2Error;

    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "vr[]", vr))
        return fmi2Error;

    if (nvr>0 && nullPointer(comp, "fmi2SetBoolean", "value[]", value))
        return fmi2Error;

    SET_BOOLEAN_VARIABLES
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetString", MASK_fmi2SetString))
        return fmi2Error;

    if (nvr>0 && nullPointer(comp, "fmi2SetString", "vr[]", vr))
        return fmi2Error;

    if (nvr>0 && nullPointer(comp, "fmi2SetString", "value[]", value))
        return fmi2Error;

    SET_VARIABLES(String)
}

fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate) {
    ModelInstance *comp = (ModelInstance *)c;
    ModelData *modelData = comp->allocateMemory(1, sizeof(ModelData));
    memcpy(modelData, comp->modelData, sizeof(ModelData));
    *FMUstate = modelData;
    return fmi2OK;
}

fmi2Status fmi2SetFMUstate (fmi2Component c, fmi2FMUstate FMUstate) {
    ModelInstance *comp = (ModelInstance *)c;
    ModelData *modelData = FMUstate;
    memcpy(comp->modelData, modelData, sizeof(ModelData));
    return fmi2OK;
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    ModelInstance *comp = (ModelInstance *)c;
    ModelData *modelData = *FMUstate;
    comp->freeMemory(modelData);
    *FMUstate = NULL;
    return fmi2OK;
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size) {
    *size = sizeof(ModelData);
    return fmi2OK;
}

fmi2Status fmi2SerializeFMUstate (fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size) {
    ModelInstance *comp = (ModelInstance *)c;
    // TODO: check size
    memcpy(serializedState, comp->modelData, sizeof(ModelData));
    return fmi2OK;
}

fmi2Status fmi2DeSerializeFMUstate (fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {

    ModelInstance *comp = (ModelInstance *)c;

    if (*FMUstate == NULL) {
        *FMUstate = comp->allocateMemory(1, sizeof(ModelData));
    }

    // TODO: check size

    memcpy(*FMUstate, serializedState, sizeof(ModelData));

    return fmi2OK;
}

fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[] , size_t nKnown,
                                        const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
    return unsupportedFunction(c, "fmi2GetDirectionalDerivative", MASK_fmi2GetDirectionalDerivative);
}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Integer order[], const fmi2Real value[]) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetRealInputDerivatives", MASK_fmi2SetRealInputDerivatives)) {
        return fmi2Error;
    }
	logError(comp, "fmi2SetRealInputDerivatives: ignoring function call."
			" This model cannot interpolate inputs: canInterpolateInputs=\"fmi2False\"");
    return fmi2Error;
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                      const fmi2Integer order[], fmi2Real value[]) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetRealOutputDerivatives", MASK_fmi2GetRealOutputDerivatives))
        return fmi2Error;
	logError(comp, "fmi2GetRealOutputDerivatives: ignoring function call."
		" This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"");
    for (i = 0; i < nvr; i++) value[i] = 0;
    return fmi2Error;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2CancelStep", MASK_fmi2CancelStep)) {
        // always fmi2CancelStep is invalid, because model is never in modelStepInProgress state.
        return fmi2Error;
    }
    logError(comp, "fmi2CancelStep: Can be called when fmi2DoStep returned fmi2Pending."
        " This is not the case.");
    return fmi2Error;
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
                    fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    ModelInstance *comp = (ModelInstance *)c;

    if (communicationStepSize <= 0) {
		logError(comp, "fmi2DoStep: communication step size must be > 0. Fount %g.", communicationStepSize);
        comp->state = modelError;
        return fmi2Error;
    }

    return doStep(comp, currentCommunicationPoint, currentCommunicationPoint + communicationStepSize);
}

/* Inquire slave status */
static fmi2Status getStatus(char* fname, fmi2Component c, const fmi2StatusKind s) {
    
    ModelInstance *comp = (ModelInstance *)c;
    
    if (invalidState(comp, fname, MASK_fmi2GetStatus)) // all get status have the same MASK_fmi2GetStatus
        return fmi2Error;

    switch(s) {
	case fmi2DoStepStatus: logError(comp,
		"%s: Can be called with fmi2DoStepStatus when fmi2DoStep returned fmi2Pending."
		" This is not the case.", fname);
        break;
	case fmi2PendingStatus: logError(comp,
		"%s: Can be called with fmi2PendingStatus when fmi2DoStep returned fmi2Pending."
		" This is not the case.", fname);
        break;
	case fmi2LastSuccessfulTime: logError(comp,
		"%s: Can be called with fmi2LastSuccessfulTime when fmi2DoStep returned fmi2Discard."
		" This is not the case.", fname);
        break;
	case fmi2Terminated: logError(comp,
		"%s: Can be called with fmi2Terminated when fmi2DoStep returned fmi2Discard."
		" This is not the case.", fname);
		break;
    }
    
    return fmi2Discard;
}

fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status *value) {
    return getStatus("fmi2GetStatus", c, s);
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real *value) {
    if (s == fmi2LastSuccessfulTime) {
        ModelInstance *comp = (ModelInstance *)c;
        if (invalidState(comp, "fmi2GetRealStatus", MASK_fmi2GetRealStatus))
            return fmi2Error;
        *value = comp->time;
        return fmi2OK;
    }
    return getStatus("fmi2GetRealStatus", c, s);
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer *value) {
    return getStatus("fmi2GetIntegerStatus", c, s);
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean *value) {
    if (s == fmi2Terminated) {
        ModelInstance *comp = (ModelInstance *)c;
        if (invalidState(comp, "fmi2GetBooleanStatus", MASK_fmi2GetBooleanStatus))
            return fmi2Error;
        *value = comp->terminateSimulation;
        return fmi2OK;
    }
    return getStatus("fmi2GetBooleanStatus", c, s);
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String *value) {
    return getStatus("fmi2GetStringStatus", c, s);
}

// ---------------------------------------------------------------------------
// Functions for FMI2 for Model Exchange
// ---------------------------------------------------------------------------
/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterEventMode", MASK_fmi2EnterEventMode))
        return fmi2Error;
    comp->state = modelEventMode;
    comp->isNewEventIteration = fmi2True;
    return fmi2OK;
}

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo) {
    ModelInstance *comp = (ModelInstance *)c;
    int timeEvent = 0;
    if (invalidState(comp, "fmi2NewDiscreteStates", MASK_fmi2NewDiscreteStates))
        return fmi2Error;
    comp->newDiscreteStatesNeeded = fmi2False;
    comp->terminateSimulation = fmi2False;
    comp->nominalsOfContinuousStatesChanged = fmi2False;
    comp->valuesOfContinuousStatesChanged = fmi2False;

    if (comp->nextEventTimeDefined && comp->nextEventTime <= comp->time) {
        timeEvent = 1;
    }

    eventUpdate(comp);

    comp->isNewEventIteration = false;

    // copy internal eventInfo of component to output eventInfo
    eventInfo->newDiscreteStatesNeeded           = comp->newDiscreteStatesNeeded;
    eventInfo->terminateSimulation               = comp->terminateSimulation;
    eventInfo->nominalsOfContinuousStatesChanged = comp->nominalsOfContinuousStatesChanged;
    eventInfo->valuesOfContinuousStatesChanged   = comp->valuesOfContinuousStatesChanged;
    eventInfo->nextEventTimeDefined              = comp->nextEventTimeDefined;
    eventInfo->nextEventTime                     = comp->nextEventTime;

    return fmi2OK;
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2EnterContinuousTimeMode", MASK_fmi2EnterContinuousTimeMode))
        return fmi2Error;
    comp->state = modelContinuousTimeMode;
    return fmi2OK;
}

fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
                                     fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2CompletedIntegratorStep", MASK_fmi2CompletedIntegratorStep))
        return fmi2Error;
    if (nullPointer(comp, "fmi2CompletedIntegratorStep", "enterEventMode", enterEventMode))
        return fmi2Error;
    if (nullPointer(comp, "fmi2CompletedIntegratorStep", "terminateSimulation", terminateSimulation))
        return fmi2Error;
    *enterEventMode = fmi2False;
    *terminateSimulation = fmi2False;
    return fmi2OK;
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2SetTime", MASK_fmi2SetTime))
        return fmi2Error;
    comp->time = time;
    return fmi2OK;
}

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2SetContinuousStates", MASK_fmi2SetContinuousStates))
        return fmi2Error;

    if (invalidNumber(comp, "fmi2SetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;

    if (nullPointer(comp, "fmi2SetContinuousStates", "x[]", x))
        return fmi2Error;

    setContinuousStates(comp, x, nx);

    return fmi2OK;
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetDerivatives", MASK_fmi2GetDerivatives))
        return fmi2Error;

    if (invalidNumber(comp, "fmi2GetDerivatives", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;

    if (nullPointer(comp, "fmi2GetDerivatives", "derivatives[]", derivatives))
        return fmi2Error;

    getDerivatives(comp, derivatives, nx);

    return fmi2OK;
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {

#if NUMBER_OF_EVENT_INDICATORS > 0
    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetEventIndicators", MASK_fmi2GetEventIndicators))
        return fmi2Error;

    if (invalidNumber(comp, "fmi2GetEventIndicators", "ni", ni, NUMBER_OF_EVENT_INDICATORS))
        return fmi2Error;

    getEventIndicators(comp, eventIndicators, ni);
#else
    if (ni > 0) return fmi2Error;
#endif
    return fmi2OK;
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real states[], size_t nx) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi2GetContinuousStates", MASK_fmi2GetContinuousStates))
        return fmi2Error;

    if (invalidNumber(comp, "fmi2GetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;

    if (nullPointer(comp, "fmi2GetContinuousStates", "states[]", states))
        return fmi2Error;

    getContinuousStates(comp, states, nx);

    return fmi2OK;
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi2GetNominalsOfContinuousStates", MASK_fmi2GetNominalsOfContinuousStates))
        return fmi2Error;
    if (invalidNumber(comp, "fmi2GetNominalContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi2Error;
    if (nullPointer(comp, "fmi2GetNominalContinuousStates", "x_nominal[]", x_nominal))
        return fmi2Error;
    for (i = 0; i < nx; i++)
        x_nominal[i] = 1;
    return fmi2OK;
}
