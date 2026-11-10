#if FMI_VERSION != 2
#error FMI_VERSION must be 2
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#ifdef _WIN32
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

#include "config.h"
#include "model.h"
#include "model_instance.h"
#include "cosimulation.h"


// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#ifndef DISABLE_PREFIX
#define pasteA(a,b)     a ## b
#define pasteB(a,b)    pasteA(a,b)
#define FMI2_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi2Functions.h"

static fmi2Status map_status(Status status) {
    switch (status) {
    case OK: return fmi2OK;
    default: return fmi2Discard;
    }
}

#define CALL(s) if ((s) != OK) { return fmi2Discard; }

#define ASSERT_NOT_NULL(p) \
do { \
    if (!p) { \
        logError(S, "Argument %s must not be NULL.", xstr(p)); \
        return fmi2Error; \
    } \
} while (false)

#define GET_VARIABLES(T) \
    size_t index = 0; \
    for (size_t i = 0; i < nvr; i++) { \
        CALL(get ## T(c, vr[i], value, nvr, &index)); \
    } \
    return fmi2OK;

#define SET_VARIABLES(T) \
    size_t index = 0; \
    for (size_t i = 0; i < nvr; i++) { \
        CALL(set ## T(c, vr[i], value, nvr, &index)); \
    } \
    return fmi2OK;

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------

static void percentDecodePath(char *path) {
    char *src = path;
    char *dst = path;
    char hex[3] = { '\0', '\0', '\0' };

    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            hex[0] = src[1];
            hex[1] = src[2];
            *dst = (char)strtol(hex, NULL, 16);
            src += 3;
        } else {
            *dst = *src;
            src++;
        }
        dst++;
    }
    *dst = '\0';
}

static bool fmi2ResourceLocationToPath(const char *resourceLocation, char *path, size_t pathSize) {

    if (!resourceLocation || !path || pathSize == 0) {
        return false;
    }

#ifdef _WIN32
    DWORD pathLen = (DWORD)pathSize;

    if (PathCreateFromUrlA(resourceLocation, path, &pathLen, 0) != S_OK) {
        return false;
    }
#else
    const char *scheme1 = "file:///";
    const char *scheme2 = "file:/";
    const char *start = NULL;

    if (strncmp(resourceLocation, scheme1, strlen(scheme1)) == 0) {
        start = &resourceLocation[strlen(scheme1)] - 1;
    } else if (strncmp(resourceLocation, scheme2, strlen(scheme2)) == 0) {
        start = &resourceLocation[strlen(scheme2)] - 1;
    } else {
        return false;
    }

    if (strlen(start) + 2 > pathSize) {
        return false;
    }

    strcpy(path, start);
    percentDecodePath(path);
#endif

    {
        const size_t len = strlen(path);

        if (len == 0 || len >= pathSize) {
            return false;
        }

#ifdef _WIN32
        if (path[len - 1] != '\\' && path[len - 1] != '/') {
            if (len + 2 > pathSize) {
                return false;
            }
            path[len] = '\\';
            path[len + 1] = '\0';
        }
#else
        if (path[len - 1] != '/') {
            if (len + 2 > pathSize) {
                return false;
            }
            path[len] = '/';
            path[len + 1] = '\0';
        }
#endif
    }

    return true;
}

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                            fmi2String fmuResourceLocation, const fmi2CallbackFunctions *functions,
                            fmi2Boolean visible, fmi2Boolean loggingOn) {

    UNUSED(visible);

    if (!functions || !functions->logger) {
        return NULL;
    }

    const char *resourcePath = NULL;
    char resourcePathBuffer[4096];

    if (fmuResourceLocation) {
        if (!fmi2ResourceLocationToPath(fmuResourceLocation, resourcePathBuffer, sizeof(resourcePathBuffer))) {
            functions->logger(
                functions->componentEnvironment,
                instanceName,
                fmi2Error,
                "error",
                "Failed to convert fmuResourceLocation to a file path.");
            return NULL;
        }
        resourcePath = resourcePathBuffer;
    }

    if (fmuType == fmi2ModelExchange) {
        return createModelExchangeInstance(
            (loggerType)functions->logger,
            functions->componentEnvironment,
            instanceName,
            fmuGUID,
            resourcePath,
            loggingOn
        );
    } else {
        return createCoSimulationInstance(
            (loggerType)functions->logger,
            NULL,
            functions->componentEnvironment,
            instanceName,
            fmuGUID,
            resourcePath,
            loggingOn,
            false,
            false
        );
    }
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                            fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {
    return map_status(setupExperiment(
        c,
        toleranceDefined != fmi2False,
        tolerance,
        startTime,
        stopTimeDefined != fmi2False,
        stopTime));
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
    return map_status(enterInitializationMode(c));
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
    return map_status(exitInitializationMode(c));
}

fmi2Status fmi2Terminate(fmi2Component c) {
    return map_status(terminate(c));
}

fmi2Status fmi2Reset(fmi2Component c) {
    return map_status(reset(c));
}

void fmi2FreeInstance(fmi2Component c) {
    freeModelInstance((ModelInstance*)c);
}

// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

const char* fmi2GetVersion(void) {
    return fmi2Version;
}

const char* fmi2GetTypesPlatform(void) {
    return fmi2TypesPlatform;
}

// ---------------------------------------------------------------------------
// FMI functions: logging control, setters and getters for Real, Integer,
// Boolean, String
// ---------------------------------------------------------------------------

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
    return map_status(setDebugLogging(c, loggingOn != fmi2False, nCategories, categories));
}

fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    GET_VARIABLES(Float64);
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    GET_VARIABLES(Int32);
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    for (size_t i = 0; i < nvr; i++) {
        bool b = false;
        size_t index = 0;
        CALL(getBoolean(c, vr[i], &b, 1, &index));
        value[i] = b ? fmi2True : fmi2False;
    }
    return fmi2OK;
}

fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    GET_VARIABLES(String);
}

fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    SET_VARIABLES(Float64);
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    SET_VARIABLES(Int32);
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    for (size_t i = 0; i < nvr; i++) {
        const bool b = value[i] != fmi2False;
        size_t index = 0;
        CALL(setBoolean(c, vr[i], &b, 1, &index));
    }
    return fmi2OK;
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    SET_VARIABLES(String);
}

fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate) {
    return map_status(getFMUState(c, FMUstate));
}

fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate FMUstate) {
    return map_status(setFMUState(c, FMUstate));
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    return map_status(freeFMUState(c, FMUstate));
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size) {
    return map_status(serializedFMUStateSize(c, FMUstate, size));
}

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size) {
    return map_status(serializeFMUState(c, FMUstate, (uint8_t *)(void *)serializedState, size));
}

fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    return map_status(deserializeFMUState(c, (const uint8_t *)(const void *)serializedState, size, (void **)FMUstate));
}

fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[] , size_t nKnown,
                                        const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {
    for (size_t i = 0; i < nUnknown; i++) {

        dvUnknown[i] = 0;
        
        for (size_t j = 0; j < nKnown; j++) {

            double partialDerivative = 0.0;
            
            CALL(getPartialDerivative(c, vUnknown_ref[i], 0, vKnown_ref[j], 0, &partialDerivative));
            
            dvUnknown[i] += partialDerivative * dvKnown[j];
        }
    }
    
    return fmi2OK;
}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Integer order[], const fmi2Real value[]) {
    UNUSED(c);
    UNUSED(vr);
    UNUSED(nvr);
    UNUSED(order);
    UNUSED(value);

    // TODO: log message

    return fmi2Discard;
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                      const fmi2Integer order[], fmi2Real value[]) {

#ifdef GET_OUTPUT_DERIVATIVE
    for (size_t i = 0; i < nvr; i++) {
        CALL(getOutputDerivative(c, vr[i], order[i], &value[i]));
    }

    return fmi2OK;
#else
    UNUSED(vr);
    UNUSED(nvr);
    UNUSED(order);
    UNUSED(value);

    logError(c, "fmi2GetRealOutputDerivatives: ignoring function call."
        " This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"");

    return fmi2Discard;
#endif
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    logError(c, "fmi2CancelStep: Can be called when fmi2DoStep returned fmi2Pending."
        " This is not the case.");
    return fmi2Discard;
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
                    fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
    
    bool eventHandlingNeeded = false;
    bool terminateSimulation = false;
    bool earlyReturn = false;
    double lastSuccessfulTime = 0.0;
    
    return map_status(doStep(
        c,
        currentCommunicationPoint,
        communicationStepSize,
        noSetFMUStatePriorToCurrentPoint != fmi2False,
        &eventHandlingNeeded,
        &terminateSimulation,
        &earlyReturn,
        &lastSuccessfulTime
    ));
}

#if 0 /* used when fmi2Get*Status TODO is implemented */
static Status getStatus(char* fname, ModelInstance* S, const fmi2StatusKind s) {

    switch(s) {
    case fmi2DoStepStatus: logError(S,
        "%s: Can be called with fmi2DoStepStatus when fmi2DoStep returned fmi2Pending."
        " This is not the case.", fname);
        break;
    case fmi2PendingStatus: logError(S,
        "%s: Can be called with fmi2PendingStatus when fmi2DoStep returned fmi2Pending."
        " This is not the case.", fname);
        break;
    case fmi2LastSuccessfulTime: logError(S,
        "%s: Can be called with fmi2LastSuccessfulTime when fmi2DoStep returned fmi2Discard."
        " This is not the case.", fname);
        break;
    case fmi2Terminated: logError(S,
        "%s: Can be called with fmi2Terminated when fmi2DoStep returned fmi2Discard."
        " This is not the case.", fname);
        break;
    }

    return Discard;
}
#endif

fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status *value) {
    UNUSED(c);
    UNUSED(s);
    UNUSED(value);
    return fmi2Discard;
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real *value) {
    UNUSED(c);
    UNUSED(s);
    UNUSED(value);
    //BEGIN_FUNCTION(GetRealStatus);

    //if (s == fmi2LastSuccessfulTime) {
    //    *value = S->time;
    //    goto TERMINATE;
    //}

    //CALL(getStatus("fmi2GetRealStatus", c, s));

    //END_FUNCTION();
    // TODO
    return fmi2Discard;
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer *value) {
    UNUSED(c);
    UNUSED(s);
    UNUSED(value);
    return fmi2Discard;
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean *value) {
    UNUSED(c);
    UNUSED(s);
    UNUSED(value);
    //BEGIN_FUNCTION(GetBooleanStatus);

    //if (s == fmi2Terminated) {
    //    *value = S->terminateSimulation;
    //    goto TERMINATE;
    //}

    //CALL(getStatus("fmi2GetBooleanStatus", c, s));

    //END_FUNCTION();
    
    // TODO
    return fmi2Discard;
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String *value) {
    UNUSED(c);
    UNUSED(s);
    UNUSED(value);
    return fmi2Discard;
}

// ---------------------------------------------------------------------------
// Functions for FMI2 for Model Exchange
// ---------------------------------------------------------------------------
/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) {
    return map_status(enterEventMode(c));
}

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo) {
#ifdef EVENT_UPDATE
    bool discreteStatesNeedUpdate = false;
    bool terminateSimulation = false;
    bool nominalsOfContinuousStatesChanged = false;
    bool valuesOfContinuousStatesChanged = false;
    bool nextEventTimeDefined = false;
    double nextEventTime = 0.0;

    const Status status = updateDiscreteStates(
        c,
        &discreteStatesNeedUpdate,
        &terminateSimulation,
        &nominalsOfContinuousStatesChanged,
        &valuesOfContinuousStatesChanged,
        &nextEventTimeDefined,
        &nextEventTime
    );

    eventInfo->newDiscreteStatesNeeded = discreteStatesNeedUpdate ? fmi2True : fmi2False;
    eventInfo->terminateSimulation = terminateSimulation ? fmi2True : fmi2False;
    eventInfo->nominalsOfContinuousStatesChanged = nominalsOfContinuousStatesChanged ? fmi2True : fmi2False;
    eventInfo->valuesOfContinuousStatesChanged = valuesOfContinuousStatesChanged ? fmi2True : fmi2False;
    eventInfo->nextEventTimeDefined = nextEventTimeDefined ? fmi2True : fmi2False;
    eventInfo->nextEventTime = nextEventTime;

    return map_status(status);
#else
    UNUSED(c);
    UNUSED(eventInfo);
    return fmi2OK;
#endif
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {
    return map_status(enterContinuousTimeMode(c));
}

fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
                                     fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {
    bool enterEvent = false;
    bool terminate = false;

    const Status status = completedIntegratorStep(
        c,
        noSetFMUStatePriorToCurrentPoint != fmi2False,
        &enterEvent,
        &terminate);

    if (enterEventMode) {
        *enterEventMode = enterEvent ? fmi2True : fmi2False;
    }
    if (terminateSimulation) {
        *terminateSimulation = terminate ? fmi2True : fmi2False;
    }

    return map_status(status);
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {
    return map_status(setTime(c, time));
}

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){
    return map_status(setContinuousStates(c, x, nx));
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {
    return map_status(getDerivatives(c, derivatives, nx));
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {
    return map_status(getEventIndicators(c, eventIndicators, ni));
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real states[], size_t nx) {
    return map_status(getContinuousStates(c, states, nx));
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {
    return map_status(getNominalsOfContinuousStates(c, x_nominal, nx));
}
