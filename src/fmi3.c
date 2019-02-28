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


// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#ifndef DISABLE_PREFIX
#define pasteA(a,b)     a ## b
#define pasteB(a,b)    pasteA(a,b)
#define fmi3_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi3Functions.h"


// macro to be used to log messages. The macro check if current
// log category is valid and, if true, call the logger provided by simulator.
#define FILTERED_LOG(instance, status, categoryIndex, message, ...) if (status == fmi3Error || status == fmi3Fatal || isCategoryLogged(instance, categoryIndex)) \
        instance->logger(instance->componentEnvironment, instance->instanceName, status, \
        logCategoriesNames[categoryIndex], message, ##__VA_ARGS__);

static const char *logCategoriesNames[] = {"logAll", "logError", "logFmiCall", "logEvent"};

#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Function calls allowed state masks for both Model-exchange and Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi3GetTypesPlatform        (modelStartAndEnd | modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepInProgress | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi3GetVersion              MASK_fmi3GetTypesPlatform
#define MASK_fmi3SetDebugLogging         (modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepInProgress | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi3Instantiate             (modelStartAndEnd)
#define MASK_fmi3FreeInstance            (modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi3SetupExperiment         modelInstantiated
#define MASK_fmi3EnterInitializationMode modelInstantiated
#define MASK_fmi3ExitInitializationMode  modelInitializationMode
#define MASK_fmi3Terminate               (modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed)
#define MASK_fmi3Reset                   MASK_fmi3FreeInstance
#define MASK_fmi3GetReal                 (modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi3GetInteger              MASK_fmi3GetReal
#define MASK_fmi3GetBoolean              MASK_fmi3GetReal
#define MASK_fmi3GetString               MASK_fmi3GetReal
#define MASK_fmi3SetReal                 (modelInstantiated | modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete)
#define MASK_fmi3SetInteger              (modelInstantiated | modelInitializationMode \
| modelEventMode \
| modelStepComplete)
#define MASK_fmi3SetBoolean              MASK_fmi3SetInteger
#define MASK_fmi3SetString               MASK_fmi3SetInteger
#define MASK_fmi3GetFMUstate             MASK_fmi3FreeInstance
#define MASK_fmi3SetFMUstate             MASK_fmi3FreeInstance
#define MASK_fmi3FreeFMUstate            MASK_fmi3FreeInstance
#define MASK_fmi3SerializedFMUstateSize  MASK_fmi3FreeInstance
#define MASK_fmi3SerializeFMUstate       MASK_fmi3FreeInstance
#define MASK_fmi3DeSerializeFMUstate     MASK_fmi3FreeInstance
#define MASK_fmi3GetDirectionalDerivative (modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Model-exchange
// ---------------------------------------------------------------------------
#define MASK_fmi3EnterEventMode          (modelEventMode | modelContinuousTimeMode)
#define MASK_fmi3NewDiscreteStates       modelEventMode
#define MASK_fmi3EnterContinuousTimeMode modelEventMode
#define MASK_fmi3CompletedIntegratorStep modelContinuousTimeMode
#define MASK_fmi3SetTime                 (modelEventMode | modelContinuousTimeMode)
#define MASK_fmi3SetContinuousStates     modelContinuousTimeMode
#define MASK_fmi3GetEventIndicators      (modelInitializationMode \
| modelEventMode | modelContinuousTimeMode \
| modelTerminated | modelError)
#define MASK_fmi3GetContinuousStates     MASK_fmi3GetEventIndicators
#define MASK_fmi3GetDerivatives          (modelEventMode | modelContinuousTimeMode \
| modelTerminated | modelError)
#define MASK_fmi3GetNominalsOfContinuousStates ( modelInstantiated \
| modelEventMode | modelContinuousTimeMode \
| modelTerminated | modelError)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi3SetRealInputDerivatives (modelInstantiated | modelInitializationMode \
| modelStepComplete)
#define MASK_fmi3GetRealOutputDerivatives (modelStepComplete | modelStepFailed | modelStepCanceled \
| modelTerminated | modelError)
#define MASK_fmi3DoStep                  modelStepComplete
#define MASK_fmi3CancelStep              modelStepInProgress
#define MASK_fmi3GetStatus               (modelStepComplete | modelStepInProgress | modelStepFailed \
| modelTerminated)
#define MASK_fmi3GetRealStatus           MASK_fmi3GetStatus
#define MASK_fmi3GetIntegerStatus        MASK_fmi3GetStatus
#define MASK_fmi3GetBooleanStatus        MASK_fmi3GetStatus
#define MASK_fmi3GetStringStatus         MASK_fmi3GetStatus

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

    comp->logger(comp->componentEnvironment, comp->instanceName, fmi3Error, "logError", buf);

    free(buf);
}

void *allocateMemory(ModelInstance *comp, size_t size) {
    return comp->allocateMemory(comp->componentEnvironment, size, 1);
}

void freeMemory(ModelInstance *comp, void *obj) {
    comp->freeMemory(comp->componentEnvironment, obj);
}

const char *duplicateString(ModelInstance *comp, const char *str1) {
    size_t len = strlen(str1);
    char *str2 = allocateMemory(comp, len + 1);
    strncpy(str2, str1, len + 1);
    return str2;
}

fmi3Boolean isCategoryLogged(ModelInstance *comp, int categoryIndex);

static bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, int n, int nExpected) {
    if (n != nExpected) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi3Error, LOG_ERROR, "%s: Invalid argument %s = %d. Expected %d.", f, arg, n, nExpected)
        return fmi3True;
    }
    return fmi3False;
}

static fmi3Boolean invalidState(ModelInstance *comp, const char *f, int statesExpected) {
    if (!comp)
        return fmi3True;
    if (!(comp->state & statesExpected)) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi3Error, LOG_ERROR, "%s: Illegal call sequence.", f)
        return fmi3True;
    }
    return fmi3False;
}

static fmi3Boolean nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {
    if (!p) {
        comp->state = modelError;
        FILTERED_LOG(comp, fmi3Error, LOG_ERROR, "%s: Invalid argument %s = NULL.", f, arg)
        return fmi3True;
    }
    return fmi3False;
}

static fmi3Status unsupportedFunction(fmi3Component c, const char *fName, int statesExpected) {
    ModelInstance *comp = (ModelInstance *)c;
    //fmi3CallbackLogger log = comp->functions->logger;
    if (invalidState(comp, fName, statesExpected))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, fName);
    FILTERED_LOG(comp, fmi3Error, LOG_ERROR, "%s: Function not implemented.", fName)
    return fmi3Error;
}

// ---------------------------------------------------------------------------
// Private helpers logger
// ---------------------------------------------------------------------------

// return fmi3True if logging category is on. Else return fmi3False.
fmi3Boolean isCategoryLogged(ModelInstance *comp, int categoryIndex) {
    if (categoryIndex < NUMBER_OF_CATEGORIES
        && (comp->logCategories[categoryIndex] || comp->logCategories[LOG_ALL])) {
        return fmi3True;
    }
    return fmi3False;
}

// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------
fmi3Component fmi3Instantiate(fmi3String instanceName, fmi3Type fmuType, fmi3String fmuGUID,
                            fmi3String fmuResourceLocation, const fmi3CallbackFunctions *functions,
                            fmi3Boolean visible, fmi3Boolean loggingOn) {
    // ignoring arguments: fmuResourceLocation, visible
    ModelInstance *comp;

    if (!functions->logger) {
        return NULL;
    }

    if (!functions->allocateMemory || !functions->freeMemory) {
        functions->logger(functions->componentEnvironment, instanceName, fmi3Error, "error",
                "fmi3Instantiate: Missing callback function.");
        return NULL;
    }

    if (!instanceName || strlen(instanceName) == 0) {
        functions->logger(functions->componentEnvironment, "?", fmi3Error, "error",
                "fmi3Instantiate: Missing instance name.");
        return NULL;
    }

    if (!fmuGUID || strlen(fmuGUID) == 0) {
        functions->logger(functions->componentEnvironment, instanceName, fmi3Error, "error",
                "fmi3Instantiate: Missing GUID.");
        return NULL;
    }

    if (strcmp(fmuGUID, MODEL_GUID)) {
//        functions->logger(functions->componentEnvironment, instanceName, fmi3Error, "error",
//                "fmi3Instantiate: Wrong GUID %s. Expected %s.", fmuGUID, MODEL_GUID);
        return NULL;
    }

    comp = (ModelInstance *)functions->allocateMemory(NULL, 1, sizeof(ModelInstance));

    if (comp) {
        int i;
        comp->instanceName = (char *)functions->allocateMemory(NULL, 1 + strlen(instanceName), sizeof(char));
        comp->GUID = (char *)functions->allocateMemory(NULL, 1 + strlen(fmuGUID), sizeof(char));
        comp->resourceLocation = (char *)functions->allocateMemory(NULL, 1 + strlen(fmuResourceLocation), sizeof(char));

        comp->modelData = (ModelData *)functions->allocateMemory(NULL, 1, sizeof(ModelData));

        // set all categories to on or off. fmi3SetDebugLogging should be called to choose specific categories.
        for (i = 0; i < NUMBER_OF_CATEGORIES; i++) {
            comp->logCategories[i] = loggingOn;
        }
    }

    if (!comp || !comp->modelData || !comp->instanceName || !comp->GUID) {
        functions->logger(functions->componentEnvironment, instanceName, fmi3Error, "error",
            "fmi3Instantiate: Out of memory.");
        return NULL;
    }

    comp->time = 0; // overwrite in fmi3SetupExperiment, fmi3SetTime
    strcpy((char *)comp->instanceName, (char *)instanceName);
    comp->type = fmuType;
    strcpy((char *)comp->GUID, (char *)fmuGUID);
    strcpy((char *)comp->resourceLocation, (char *)fmuResourceLocation);
    comp->logger = functions->logger;
    comp->allocateMemory = functions->allocateMemory;
    comp->freeMemory = functions->freeMemory;
    comp->stepFinished = functions->stepFinished;
    comp->componentEnvironment = functions->componentEnvironment;
    comp->loggingOn = loggingOn;
    comp->state = modelInstantiated;
    comp->isNewEventIteration = fmi3False;

    comp->newDiscreteStatesNeeded = fmi3False;
    comp->terminateSimulation = fmi3False;
    comp->nominalsOfContinuousStatesChanged = fmi3False;
    comp->valuesOfContinuousStatesChanged = fmi3False;
    comp->nextEventTimeDefined = fmi3False;
    comp->nextEventTime = 0;

    setStartValues(comp);
    comp->isDirtyValues = true;

    comp->solverData = solver_create(comp);

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3Instantiate: GUID=%s", fmuGUID)

    return comp;
}

fmi3Status fmi3SetupExperiment(fmi3Component c, fmi3Boolean toleranceDefined, fmi3Float64 tolerance,
                            fmi3Float64 startTime, fmi3Boolean stopTimeDefined, fmi3Float64 stopTime) {

    // ignore arguments: stopTimeDefined, stopTime
    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3SetupExperiment", MASK_fmi3SetupExperiment))
        return fmi3Error;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetupExperiment: toleranceDefined=%d tolerance=%g",
        toleranceDefined, tolerance)

    comp->time = startTime;

    return fmi3OK;
}

fmi3Status fmi3EnterInitializationMode(fmi3Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3EnterInitializationMode", MASK_fmi3EnterInitializationMode))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3EnterInitializationMode")

    comp->state = modelInitializationMode;
    return fmi3OK;
}

fmi3Status fmi3ExitInitializationMode(fmi3Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3ExitInitializationMode", MASK_fmi3ExitInitializationMode))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3ExitInitializationMode")

    // if values were set and no fmi3GetXXX triggered update before,
    // ensure calculated values are updated now
    if (comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    if (comp->type == fmi3ModelExchange) {
        comp->state = modelEventMode;
        comp->isNewEventIteration = fmi3True;
    } else {
        comp->state = modelStepComplete;
    }

    return fmi3OK;
}

fmi3Status fmi3Terminate(fmi3Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3Terminate", MASK_fmi3Terminate))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3Terminate")

    comp->state = modelTerminated;
    return fmi3OK;
}

fmi3Status fmi3Reset(fmi3Component c) {
    ModelInstance* comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3Reset", MASK_fmi3Reset))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3Reset")

    comp->state = modelInstantiated;
    setStartValues(comp);
    comp->isDirtyValues = true;
    return fmi3OK;
}

void fmi3FreeInstance(fmi3Component c) {

    ModelInstance *comp = (ModelInstance *)c;

    if (!comp) return;

    if (invalidState(comp, "fmi3FreeInstance", MASK_fmi3FreeInstance))
        return;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3FreeInstance")

    if (comp->instanceName) comp->freeMemory(comp, (void *)comp->instanceName);
    if (comp->GUID) comp->freeMemory(comp, (void *)comp->GUID);
    comp->freeMemory(comp, comp);
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmi3GetVersion() {
    return fmi3Version;
}

// ---------------------------------------------------------------------------
// FMI functions: logging control, setters and getters for Real, Integer,
// Boolean, String
// ---------------------------------------------------------------------------

fmi3Status fmi3SetDebugLogging(fmi3Component c, fmi3Boolean loggingOn, size_t nCategories, const fmi3String categories[]) {
    int i, j;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3SetDebugLogging", MASK_fmi3SetDebugLogging))
        return fmi3Error;
    comp->loggingOn = loggingOn;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetDebugLogging")

    // reset all categories
    for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
        comp->logCategories[j] = fmi3False;
    }

    if (nCategories == 0) {
        // no category specified, set all categories to have loggingOn value
        for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
            comp->logCategories[j] = loggingOn;
        }
    } else {
        // set specific categories on
        for (i = 0; i < nCategories; i++) {
            fmi3Boolean categoryFound = fmi3False;
            for (j = 0; j < NUMBER_OF_CATEGORIES; j++) {
                if (strcmp(logCategoriesNames[j], categories[i]) == 0) {
                    comp->logCategories[j] = loggingOn;
                    categoryFound = fmi3True;
                    break;
                }
            }
            if (!categoryFound) {
                comp->logger(comp->componentEnvironment, comp->instanceName, fmi3Warning,
                    logCategoriesNames[LOG_ERROR],
                    "logging category '%s' is not supported by model", categories[i]);
            }
        }
    }
    return fmi3OK;
}

fmi3Status fmi3GetFloat64 (fmi3Component c, const fmi3ValueReference vr[], size_t nvr, fmi3Float64 value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetReal", MASK_fmi3GetReal))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3GetReal", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3GetReal", "value[]", value))
        return fmi3Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_VARIABLES(Float64)
}

fmi3Status fmi3GetInt32(fmi3Component c, const fmi3ValueReference vr[], size_t nvr, fmi3Int32 value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetInteger", MASK_fmi3GetInteger))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3GetInteger", "vr[]", vr))
            return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3GetInteger", "value[]", value))
            return fmi3Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_VARIABLES(Int32)
}

fmi3Status fmi3GetBoolean(fmi3Component c, const fmi3ValueReference vr[], size_t nvr, fmi3Boolean value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetBoolean", MASK_fmi3GetBoolean))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3GetBoolean", "vr[]", vr))
            return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3GetBoolean", "value[]", value))
            return fmi3Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_BOOLEAN_VARIABLES
}

fmi3Status fmi3GetString (fmi3Component c, const fmi3ValueReference vr[], size_t nvr, fmi3String value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetString", MASK_fmi3GetString))
        return fmi3Error;

    if (nvr>0 && nullPointer(comp, "fmi3GetString", "vr[]", vr))
            return fmi3Error;

    if (nvr>0 && nullPointer(comp, "fmi3GetString", "value[]", value))
            return fmi3Error;

    if (nvr > 0 && comp->isDirtyValues) {
        calculateValues(comp);
        comp->isDirtyValues = false;
    }

    GET_VARIABLES(String)
}

fmi3Status fmi3SetFloat64 (fmi3Component c, const fmi3ValueReference vr[], size_t nvr, const fmi3Float64 value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3SetReal", MASK_fmi3SetReal))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3SetReal", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3SetReal", "value[]", value))
        return fmi3Error;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetReal: nvr = %d", nvr)

    SET_VARIABLES(Float64)
}

fmi3Status fmi3SetInt32(fmi3Component c, const fmi3ValueReference vr[], size_t nvr, const fmi3Int32 value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3SetInteger", MASK_fmi3SetInteger))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3SetInteger", "vr[]", vr))
        return fmi3Error;

    if (nvr > 0 && nullPointer(comp, "fmi3SetInteger", "value[]", value))
        return fmi3Error;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetInteger: nvr = %d", nvr)

    SET_VARIABLES(Int32)
}

fmi3Status fmi3SetBoolean(fmi3Component c, const fmi3ValueReference vr[], size_t nvr, const fmi3Boolean value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3SetBoolean", MASK_fmi3SetBoolean))
        return fmi3Error;

    if (nvr>0 && nullPointer(comp, "fmi3SetBoolean", "vr[]", vr))
        return fmi3Error;

    if (nvr>0 && nullPointer(comp, "fmi3SetBoolean", "value[]", value))
        return fmi3Error;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetBoolean: nvr = %d", nvr)

    SET_BOOLEAN_VARIABLES
}

fmi3Status fmi3SetString (fmi3Component c, const fmi3ValueReference vr[], size_t nvr, const fmi3String value[], size_t nValues) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3SetString", MASK_fmi3SetString))
        return fmi3Error;

    if (nvr>0 && nullPointer(comp, "fmi3SetString", "vr[]", vr))
        return fmi3Error;

    if (nvr>0 && nullPointer(comp, "fmi3SetString", "value[]", value))
        return fmi3Error;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetString: nvr = %d", nvr)

    SET_VARIABLES(String)
}

fmi3Status fmi3GetFMUstate (fmi3Component c, fmi3FMUstate* FMUstate) {
    return unsupportedFunction(c, "fmi3GetFMUstate", MASK_fmi3GetFMUstate);
}
fmi3Status fmi3SetFMUstate (fmi3Component c, fmi3FMUstate FMUstate) {
    return unsupportedFunction(c, "fmi3SetFMUstate", MASK_fmi3SetFMUstate);
}
fmi3Status fmi3FreeFMUstate(fmi3Component c, fmi3FMUstate* FMUstate) {
    return unsupportedFunction(c, "fmi3FreeFMUstate", MASK_fmi3FreeFMUstate);
}
fmi3Status fmi3SerializedFMUstateSize(fmi3Component c, fmi3FMUstate FMUstate, size_t *size) {
    return unsupportedFunction(c, "fmi3SerializedFMUstateSize", MASK_fmi3SerializedFMUstateSize);
}
fmi3Status fmi3SerializeFMUstate (fmi3Component c, fmi3FMUstate FMUstate, fmi3Byte serializedState[], size_t size) {
    return unsupportedFunction(c, "fmi3SerializeFMUstate", MASK_fmi3SerializeFMUstate);
}
fmi3Status fmi3DeSerializeFMUstate (fmi3Component c, const fmi3Byte serializedState[], size_t size,
                                    fmi3FMUstate* FMUstate) {
    return unsupportedFunction(c, "fmi3DeSerializeFMUstate", MASK_fmi3DeSerializeFMUstate);
}

//fmi3Status fmi3GetDirectionalDerivative(fmi3Component c, const fmi3ValueReference vUnknown_ref[], size_t nUnknown,
//                                        const fmi3ValueReference vKnown_ref[] , size_t nKnown,
//                                        const fmi3Float64 dvKnown[], fmi3Float64 dvUnknown[]) {
//    return unsupportedFunction(c, "fmi3GetDirectionalDerivative", MASK_fmi3GetDirectionalDerivative);
//}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
/* Simulating the slave */
//fmi3Status fmi3SetRealInputDerivatives(fmi3Component c, const fmi3ValueReference vr[], size_t nvr,
//                                     const fmi3Integer order[], const fmi3Float64 value[]) {
//    ModelInstance *comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmi3SetRealInputDerivatives", MASK_fmi3SetRealInputDerivatives)) {
//        return fmi3Error;
//    }
//    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetRealInputDerivatives: nvr= %d", nvr)
//    FILTERED_LOG(comp, fmi3Error, LOG_ERROR, "fmi3SetRealInputDerivatives: ignoring function call."
//        " This model cannot interpolate inputs: canInterpolateInputs=\"fmi3False\"")
//    return fmi3Error;
//}

//fmi3Status fmi3GetRealOutputDerivatives(fmi3Component c, const fmi3ValueReference vr[], size_t nvr,
//                                      const fmi3Integer order[], fmi3Float64 value[]) {
//    int i;
//    ModelInstance *comp = (ModelInstance *)c;
//    if (invalidState(comp, "fmi3GetRealOutputDerivatives", MASK_fmi3GetRealOutputDerivatives))
//        return fmi3Error;
//    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3GetRealOutputDerivatives: nvr= %d", nvr)
//    FILTERED_LOG(comp, fmi3Error, LOG_ERROR,"fmi3GetRealOutputDerivatives: ignoring function call."
//        " This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"")
//    for (i = 0; i < nvr; i++) value[i] = 0;
//    return fmi3Error;
//}

fmi3Status fmi3CancelStep(fmi3Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3CancelStep", MASK_fmi3CancelStep)) {
        // always fmi3CancelStep is invalid, because model is never in modelStepInProgress state.
        return fmi3Error;
    }
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3CancelStep")
    FILTERED_LOG(comp, fmi3Error, LOG_ERROR,"fmi3CancelStep: Can be called when fmi3DoStep returned fmi3Pending."
        " This is not the case.");
    // comp->state = modelStepCanceled;
    return fmi3Error;
}

fmi3Status fmi3DoStep(fmi3Component c, fmi3Float64 currentCommunicationPoint,
                    fmi3Float64 communicationStepSize, fmi3Boolean noSetFMUStatePriorToCurrentPoint) {
    ModelInstance *comp = (ModelInstance *)c;

    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3DoStep: "
        "currentCommunicationPoint = %g, "
        "communicationStepSize = %g, "
        "noSetFMUStatePriorToCurrentPoint = fmi3%s",
        currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint ? "True" : "False")

    if (communicationStepSize <= 0) {
        FILTERED_LOG(comp, fmi3Error, LOG_ERROR,
            "fmi3DoStep: communication step size must be > 0. Fount %g.", communicationStepSize)
        comp->state = modelError;
        return fmi3Error;
    }

    return doStep(comp, currentCommunicationPoint, currentCommunicationPoint + communicationStepSize);
}

// ---------------------------------------------------------------------------
// Functions for fmi3 for Model Exchange
// ---------------------------------------------------------------------------
/* Enter and exit the different modes */
fmi3Status fmi3EnterEventMode(fmi3Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3EnterEventMode", MASK_fmi3EnterEventMode))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3EnterEventMode")

    comp->state = modelEventMode;
    comp->isNewEventIteration = fmi3True;
    return fmi3OK;
}

fmi3Status fmi3NewDiscreteStates(fmi3Component c, fmi3EventInfo *eventInfo) {
    ModelInstance *comp = (ModelInstance *)c;
    int timeEvent = 0;
    if (invalidState(comp, "fmi3NewDiscreteStates", MASK_fmi3NewDiscreteStates))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3NewDiscreteStates")

    comp->newDiscreteStatesNeeded = fmi3False;
    comp->terminateSimulation = fmi3False;
    comp->nominalsOfContinuousStatesChanged = fmi3False;
    comp->valuesOfContinuousStatesChanged = fmi3False;

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

    return fmi3OK;
}

fmi3Status fmi3EnterContinuousTimeMode(fmi3Component c) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3EnterContinuousTimeMode", MASK_fmi3EnterContinuousTimeMode))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL,"fmi3EnterContinuousTimeMode")

    comp->state = modelContinuousTimeMode;
    return fmi3OK;
}

fmi3Status fmi3CompletedIntegratorStep(fmi3Component c, fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                                     fmi3Boolean *enterEventMode, fmi3Boolean *terminateSimulation) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3CompletedIntegratorStep", MASK_fmi3CompletedIntegratorStep))
        return fmi3Error;
    if (nullPointer(comp, "fmi3CompletedIntegratorStep", "enterEventMode", enterEventMode))
        return fmi3Error;
    if (nullPointer(comp, "fmi3CompletedIntegratorStep", "terminateSimulation", terminateSimulation))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL,"fmi3CompletedIntegratorStep")
    *enterEventMode = fmi3False;
    *terminateSimulation = fmi3False;
    return fmi3OK;
}

/* Providing independent variables and re-initialization of caching */
fmi3Status fmi3SetTime(fmi3Component c, fmi3Float64 time) {
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3SetTime", MASK_fmi3SetTime))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3SetTime: time=%.16g", time)
    comp->time = time;
    return fmi3OK;
}

fmi3Status fmi3SetContinuousStates(fmi3Component c, const fmi3Float64 x[], size_t nx){

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3SetContinuousStates", MASK_fmi3SetContinuousStates))
        return fmi3Error;

    if (invalidNumber(comp, "fmi3SetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(comp, "fmi3SetContinuousStates", "x[]", x))
        return fmi3Error;

    setContinuousStates(comp, x, nx);

    return fmi3OK;
}

/* Evaluation of the model equations */
fmi3Status fmi3GetDerivatives(fmi3Component c, fmi3Float64 derivatives[], size_t nx) {

    ModelInstance* comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetDerivatives", MASK_fmi3GetDerivatives))
        return fmi3Error;

    if (invalidNumber(comp, "fmi3GetDerivatives", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(comp, "fmi3GetDerivatives", "derivatives[]", derivatives))
        return fmi3Error;

    getDerivatives(comp, derivatives, nx);

    return fmi3OK;
}

fmi3Status fmi3GetEventIndicators(fmi3Component c, fmi3Float64 eventIndicators[], size_t ni) {

#if NUMBER_OF_EVENT_INDICATORS > 0
    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetEventIndicators", MASK_fmi3GetEventIndicators))
        return fmi3Error;

    if (invalidNumber(comp, "fmi3GetEventIndicators", "ni", ni, NUMBER_OF_EVENT_INDICATORS))
        return fmi3Error;

    getEventIndicators(comp, eventIndicators, ni);
#else
    if (ni > 0) return fmi3Error;
#endif
    return fmi3OK;
}

fmi3Status fmi3GetContinuousStates(fmi3Component c, fmi3Float64 states[], size_t nx) {

    ModelInstance *comp = (ModelInstance *)c;

    if (invalidState(comp, "fmi3GetContinuousStates", MASK_fmi3GetContinuousStates))
        return fmi3Error;

    if (invalidNumber(comp, "fmi3GetContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;

    if (nullPointer(comp, "fmi3GetContinuousStates", "states[]", states))
        return fmi3Error;

    getContinuousStates(comp, states, nx);

    return fmi3OK;
}

fmi3Status fmi3GetNominalsOfContinuousStates(fmi3Component c, fmi3Float64 x_nominal[], size_t nx) {
    int i;
    ModelInstance *comp = (ModelInstance *)c;
    if (invalidState(comp, "fmi3GetNominalsOfContinuousStates", MASK_fmi3GetNominalsOfContinuousStates))
        return fmi3Error;
    if (invalidNumber(comp, "fmi3GetNominalContinuousStates", "nx", nx, NUMBER_OF_STATES))
        return fmi3Error;
    if (nullPointer(comp, "fmi3GetNominalContinuousStates", "x_nominal[]", x_nominal))
        return fmi3Error;
    FILTERED_LOG(comp, fmi3OK, LOG_FMI_CALL, "fmi3GetNominalContinuousStates: x_nominal[0..%d] = 1.0", nx-1)
    for (i = 0; i < nx; i++)
        x_nominal[i] = 1;
    return fmi3OK;
}
