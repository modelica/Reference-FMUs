#if FMI_VERSION != 2
#error FMI_VERSION must be 2
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#include "config.h"
#include "model.h"
#include "cosimulation.h"


// C-code FMUs have functions names prefixed with MODEL_IDENTIFIER_.
// Define DISABLE_PREFIX to build a binary FMU.
#ifndef DISABLE_PREFIX
#define pasteA(a,b)     a ## b
#define pasteB(a,b)    pasteA(a,b)
#define FMI2_FUNCTION_PREFIX pasteB(MODEL_IDENTIFIER, _)
#endif
#include "fmi2Functions.h"

#define ASSERT_NOT_NULL(p) \
do { \
    if (!p) { \
        logError(S, "Argument %s must not be NULL.", xstr(p)); \
        S->state = Terminated; \
        return (fmi2Status)Error; \
    } \
} while (false)

#define GET_VARIABLES(T) \
do { \
    if (nvr == 0) goto TERMINATE; \
    ASSERT_NOT_NULL(vr); \
    ASSERT_NOT_NULL(value); \
    size_t index = 0; \
    if (S->isDirtyValues) { \
        CALL(calculateValues(S)); \
        S->isDirtyValues = false; \
    } \
    for (size_t i = 0; i < nvr; i++) { \
        CALL(get ## T(S, vr[i], value, nvr, &index)); \
    } \
} while (false)

#define SET_VARIABLES(T) \
do { \
    if (nvr == 0) goto TERMINATE; \
    ASSERT_NOT_NULL(vr); \
    ASSERT_NOT_NULL(value); \
    size_t index = 0; \
    for (size_t i = 0; i < nvr; i++) { \
        CALL(set ## T(S, vr[i], value, nvr, &index)); \
    } \
    if (nvr > 0) S->isDirtyValues = true; \
} while (false)

#define GET_BOOLEAN_VARIABLES \
do { \
    for (size_t i = 0; i < nvr; i++) { \
        bool v; \
        size_t index = 0; \
        CALL(getBoolean(S, vr[i], &v, nvr, &index)); \
        value[i] = v; \
    } \
} while (false)

#define SET_BOOLEAN_VARIABLES \
do { \
    for (size_t i = 0; i < nvr; i++) { \
        const bool v = value[i]; \
        size_t index = 0; \
        CALL(setBoolean(S, vr[i], &v, nvr, &index)); \
    } \
} while (false)

#ifndef DT_EVENT_DETECT
#define DT_EVENT_DETECT 1e-10
#endif

// ---------------------------------------------------------------------------
// Function calls allowed state masks for both Model-exchange and Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi2GetTypesPlatform        (StartAndEnd | Instantiated | InitializationMode \
| EventMode | ContinuousTimeMode \
| StepComplete | StepInProgress | StepFailed | StepCanceled \
| Terminated)
#define MASK_fmi2GetVersion              MASK_fmi2GetTypesPlatform
#define MASK_fmi2SetDebugLogging         (Instantiated | InitializationMode \
| EventMode | ContinuousTimeMode \
| StepComplete | StepInProgress | StepFailed | StepCanceled \
| Terminated)
#define MASK_fmi2Instantiate             (StartAndEnd)
#define MASK_fmi2FreeInstance            (Instantiated | InitializationMode \
| EventMode | ContinuousTimeMode \
| StepComplete | StepFailed | StepCanceled \
| Terminated)
#define MASK_fmi2SetupExperiment         Instantiated
#define MASK_fmi2EnterInitializationMode Instantiated
#define MASK_fmi2ExitInitializationMode  InitializationMode
#define MASK_fmi2Terminate               (EventMode | ContinuousTimeMode \
| StepComplete | StepFailed)
#define MASK_fmi2Reset                   MASK_fmi2FreeInstance
#define MASK_fmi2GetReal                 (InitializationMode \
| EventMode | ContinuousTimeMode \
| StepComplete | StepFailed | StepCanceled \
| Terminated)
#define MASK_fmi2GetInteger              MASK_fmi2GetReal
#define MASK_fmi2GetBoolean              MASK_fmi2GetReal
#define MASK_fmi2GetString               MASK_fmi2GetReal
#define MASK_fmi2SetReal                 (Instantiated | InitializationMode \
| EventMode | ContinuousTimeMode \
| StepComplete)
#define MASK_fmi2SetInteger              (Instantiated | InitializationMode \
| EventMode \
| StepComplete)
#define MASK_fmi2SetBoolean              MASK_fmi2SetInteger
#define MASK_fmi2SetString               MASK_fmi2SetInteger
#define MASK_fmi2GetFMUstate             MASK_fmi2FreeInstance
#define MASK_fmi2SetFMUstate             MASK_fmi2FreeInstance
#define MASK_fmi2FreeFMUstate            MASK_fmi2FreeInstance
#define MASK_fmi2SerializedFMUstateSize  MASK_fmi2FreeInstance
#define MASK_fmi2SerializeFMUstate       MASK_fmi2FreeInstance
#define MASK_fmi2DeSerializeFMUstate     MASK_fmi2FreeInstance
#define MASK_fmi2GetDirectionalDerivative (InitializationMode \
| EventMode | ContinuousTimeMode \
| StepComplete | StepFailed | StepCanceled \
| Terminated)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Model-exchange
// ---------------------------------------------------------------------------
#define MASK_fmi2EnterEventMode          (EventMode | ContinuousTimeMode)
#define MASK_fmi2NewDiscreteStates       EventMode
#define MASK_fmi2EnterContinuousTimeMode EventMode
#define MASK_fmi2CompletedIntegratorStep ContinuousTimeMode
#define MASK_fmi2SetTime                 (EventMode | ContinuousTimeMode)
#define MASK_fmi2SetContinuousStates     ContinuousTimeMode
#define MASK_fmi2GetEventIndicators      (InitializationMode \
| EventMode | ContinuousTimeMode \
| Terminated)
#define MASK_fmi2GetContinuousStates     MASK_fmi2GetEventIndicators
#define MASK_fmi2GetDerivatives          (EventMode | ContinuousTimeMode \
| Terminated)
#define MASK_fmi2GetNominalsOfContinuousStates ( Instantiated \
| EventMode | ContinuousTimeMode \
| Terminated)

// ---------------------------------------------------------------------------
// Function calls allowed state masks for Co-simulation
// ---------------------------------------------------------------------------
#define MASK_fmi2SetRealInputDerivatives (Instantiated | InitializationMode \
| StepComplete)
#define MASK_fmi2GetRealOutputDerivatives (StepComplete | StepFailed | StepCanceled \
| Terminated | Error)
#define MASK_fmi2DoStep                  StepComplete
#define MASK_fmi2CancelStep              StepInProgress
#define MASK_fmi2GetStatus               (StepComplete | StepInProgress | StepFailed \
| Terminated)
#define MASK_fmi2GetRealStatus           MASK_fmi2GetStatus
#define MASK_fmi2GetIntegerStatus        MASK_fmi2GetStatus
#define MASK_fmi2GetBooleanStatus        MASK_fmi2GetStatus
#define MASK_fmi2GetStringStatus         MASK_fmi2GetStatus

#define BEGIN_FUNCTION(F) \
Status status = OK; \
if (!c) return fmi2Error; \
ModelInstance *S = (ModelInstance *)c; \
if (!allowedState(S, MASK_fmi2##F, #F)) CALL(Error);

#define END_FUNCTION() \
TERMINATE: \
    return (fmi2Status)status;

#define CALL(f) do { \
    const Status _s = f; \
    if (_s > status) { \
        status = _s; \
    } \
    if (status == Discard) { \
        goto TERMINATE; \
    } else if (status == Error) { \
        S->state = Terminated; \
        goto TERMINATE; \
    } else if (status == Fatal) { \
        S->state = StartAndEnd; \
        goto TERMINATE; \
    } \
} while (false)

static bool allowedState(ModelInstance *instance, int statesExpected, char *name) {

    if (!instance) {
        return false;
    }

    if (!(instance->state & statesExpected)) {
        logError(instance, "fmi2%s: Illegal call sequence.", name);
        return false;
    }

    return true;

}

// ---------------------------------------------------------------------------
// FMI functions
// ---------------------------------------------------------------------------

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID,
                            fmi2String fmuResourceLocation, const fmi2CallbackFunctions *functions,
                            fmi2Boolean visible, fmi2Boolean loggingOn) {

    UNUSED(visible);

    if (!functions || !functions->logger) {
        return NULL;
    }

    return createModelInstance(
        (loggerType)functions->logger,
        NULL,
        functions->componentEnvironment,
        instanceName,
        fmuGUID,
        fmuResourceLocation,
        loggingOn,
        (InterfaceType)fmuType);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance,
                            fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime) {

    UNUSED(toleranceDefined);
    UNUSED(tolerance);

    BEGIN_FUNCTION(SetupExperiment)

    S->startTime = startTime;
    S->stopTime = stopTimeDefined ? stopTime : INFINITY;
    S->time = startTime;
    S->nextCommunicationPoint = startTime;

    END_FUNCTION();
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {

    BEGIN_FUNCTION(EnterInitializationMode)

    S->state = InitializationMode;

    END_FUNCTION();
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {

    BEGIN_FUNCTION(ExitInitializationMode);

    // if values were set and no fmi2GetXXX triggered update before,
    // ensure calculated values are updated now
    if (S->isDirtyValues) {
        CALL(calculateValues(S));
        S->isDirtyValues = false;
    }

    S->state = S->type == ModelExchange ? EventMode : StepComplete;

    CALL(configurate(S));

    END_FUNCTION();
}

fmi2Status fmi2Terminate(fmi2Component c) {

    BEGIN_FUNCTION(Terminate)

    S->state = Terminated;

    END_FUNCTION();
}

fmi2Status fmi2Reset(fmi2Component c) {

    BEGIN_FUNCTION(Reset);

    CALL(reset(S));

    END_FUNCTION();
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

    BEGIN_FUNCTION(SetDebugLogging)

    CALL(setDebugLogging(S, loggingOn, nCategories, categories));

    END_FUNCTION();
}

fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {

    BEGIN_FUNCTION(GetReal)

    if (nvr > 0 && nullPointer(S, "fmi2GetReal", "vr[]", vr))
        return fmi2Error;

    if (nvr > 0 && nullPointer(S, "fmi2GetReal", "value[]", value))
        return fmi2Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_VARIABLES(Float64);

    END_FUNCTION();
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {

    BEGIN_FUNCTION(GetInteger)

    if (nvr > 0 && nullPointer(S, "fmi2GetInteger", "vr[]", vr))
            return fmi2Error;

    if (nvr > 0 && nullPointer(S, "fmi2GetInteger", "value[]", value))
            return fmi2Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_VARIABLES(Int32);

    END_FUNCTION();
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {

    BEGIN_FUNCTION(GetBoolean)

    if (nvr > 0 && nullPointer(S, "fmi2GetBoolean", "vr[]", vr))
            return fmi2Error;

    if (nvr > 0 && nullPointer(S, "fmi2GetBoolean", "value[]", value))
            return fmi2Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_BOOLEAN_VARIABLES;

    END_FUNCTION();
}

fmi2Status fmi2GetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {

    BEGIN_FUNCTION(GetString)

    if (nvr>0 && nullPointer(S, "fmi2GetString", "vr[]", vr))
            return fmi2Error;

    if (nvr>0 && nullPointer(S, "fmi2GetString", "value[]", value))
            return fmi2Error;

    if (nvr > 0 && S->isDirtyValues) {
        calculateValues(S);
        S->isDirtyValues = false;
    }

    GET_VARIABLES(String);

    END_FUNCTION();
}

fmi2Status fmi2SetReal (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {

    BEGIN_FUNCTION(SetReal)

    if (nvr > 0 && nullPointer(S, "fmi2SetReal", "vr[]", vr))
        return fmi2Error;

    if (nvr > 0 && nullPointer(S, "fmi2SetReal", "value[]", value))
        return fmi2Error;

    SET_VARIABLES(Float64);

    END_FUNCTION();
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {

    BEGIN_FUNCTION(SetInteger)

    if (nvr > 0 && nullPointer(S, "fmi2SetInteger", "vr[]", vr))
        return fmi2Error;

    if (nvr > 0 && nullPointer(S, "fmi2SetInteger", "value[]", value))
        return fmi2Error;

    SET_VARIABLES(Int32);

    END_FUNCTION();
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {

    BEGIN_FUNCTION(SetBoolean)

    if (nvr>0 && nullPointer(S, "fmi2SetBoolean", "vr[]", vr))
        return fmi2Error;

    if (nvr>0 && nullPointer(S, "fmi2SetBoolean", "value[]", value))
        return fmi2Error;

    SET_BOOLEAN_VARIABLES;

    END_FUNCTION();
}

fmi2Status fmi2SetString (fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {

    BEGIN_FUNCTION(SetString);

    if (nvr>0 && nullPointer(S, "fmi2SetString", "vr[]", vr))
        return fmi2Error;

    if (nvr>0 && nullPointer(S, "fmi2SetString", "value[]", value))
        return fmi2Error;

    SET_VARIABLES(String);

    END_FUNCTION();
}

fmi2Status fmi2GetFMUstate (fmi2Component c, fmi2FMUstate* FMUstate) {

    BEGIN_FUNCTION(GetFMUstate);

    getFMUState(S, FMUstate);

    END_FUNCTION();
}

fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate FMUstate) {

    BEGIN_FUNCTION(SetFMUstate);

    if (nullPointer(S, "fmi2SetFMUstate", "FMUstate", FMUstate)) {
        return fmi2Error;
    }

    setFMUState(S, FMUstate);

    END_FUNCTION();
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {

    BEGIN_FUNCTION(FreeFMUstate);

    free(*FMUstate);

    *FMUstate = NULL;

    END_FUNCTION();
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size) {

    UNUSED(c);
    UNUSED(FMUstate);

    BEGIN_FUNCTION(SerializedFMUstateSize);

    *size = sizeof(ModelInstance);

    END_FUNCTION();
}

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size) {

    BEGIN_FUNCTION(SerializeFMUstate);

    if (nullPointer(S, "fmi2SerializeFMUstate", "FMUstate", FMUstate)) {
        return fmi2Error;
    }

    if (invalidNumber(S, "fmi2SerializeFMUstate", "size", size, sizeof(ModelInstance))) {
        return fmi2Error;
    }

    memcpy(serializedState, FMUstate, sizeof(ModelInstance));

    END_FUNCTION();
}

fmi2Status fmi2DeSerializeFMUstate (fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {

    BEGIN_FUNCTION(DeSerializeFMUstate);

    if (invalidNumber(S, "fmi2DeSerializeFMUstate", "size", size, sizeof(ModelInstance))) {
        return fmi2Error;
    }

    if (*FMUstate == NULL) {
        *FMUstate = calloc(1, sizeof(ModelInstance));
    }

    memcpy(*FMUstate, serializedState, sizeof(ModelInstance));

    END_FUNCTION();
}

fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[] , size_t nKnown,
                                        const fmi2Real dvKnown[], fmi2Real dvUnknown[]) {

    BEGIN_FUNCTION(GetDirectionalDerivative);

    // TODO: check value references
    // TODO: assert nUnknowns == nDeltaOfUnknowns
    // TODO: assert nKnowns == nDeltaKnowns

    for (size_t i = 0; i < nUnknown; i++) {
        dvUnknown[i] = 0;
        for (size_t j = 0; j < nKnown; j++) {
            double partialDerivative = 0;
            CALL(getPartialDerivative(S, vUnknown_ref[i], vKnown_ref[j], &partialDerivative));
            dvUnknown[i] += partialDerivative * dvKnown[j];
        }
    }

    END_FUNCTION();
}

// ---------------------------------------------------------------------------
// Functions for FMI for Co-Simulation
// ---------------------------------------------------------------------------
/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                     const fmi2Integer order[], const fmi2Real value[]) {

    UNUSED(vr);
    UNUSED(nvr);
    UNUSED(order);
    UNUSED(value);

    BEGIN_FUNCTION(SetRealInputDerivatives);

    logError(S, "fmi2SetRealInputDerivatives: ignoring function call."
            " This model cannot interpolate inputs: canInterpolateInputs=\"fmi2False\"");
    CALL(Error);

    END_FUNCTION();
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                      const fmi2Integer order[], fmi2Real value[]) {

    BEGIN_FUNCTION(GetRealOutputDerivatives);

#ifdef GET_OUTPUT_DERIVATIVE
    for (size_t i = 0; i < nvr; i++) {
        CALL(getOutputDerivative(S, vr[i], order[i], &value[i]));
    }
#else
    UNUSED(vr);
    UNUSED(nvr);
    UNUSED(order);
    UNUSED(value);

    logError(S, "fmi2GetRealOutputDerivatives: ignoring function call."
        " This model cannot compute derivatives of outputs: MaxOutputDerivativeOrder=\"0\"");
    CALL(Error);
#endif

    END_FUNCTION();
}

fmi2Status fmi2CancelStep(fmi2Component c) {

    BEGIN_FUNCTION(CancelStep);

    logError(S, "fmi2CancelStep: Can be called when fmi2DoStep returned fmi2Pending."
        " This is not the case.");
    CALL(Error);

    END_FUNCTION();
}

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint,
                    fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {

    UNUSED(noSetFMUStatePriorToCurrentPoint);

    BEGIN_FUNCTION(DoStep);

    if (!isClose(currentCommunicationPoint, S->nextCommunicationPoint)) {
        logError(S, "Expected currentCommunicationPoint = %.16g but was %.16g.",
            S->nextCommunicationPoint, currentCommunicationPoint);
        S->state = Terminated;
        CALL(Error);
    }

    if (communicationStepSize <= 0) {
        logError(S, "Communication step size must be > 0 but was %g.", communicationStepSize);
        CALL(Error);
    }

    const fmi2Real nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;

    if (nextCommunicationPoint > S->stopTime && !isClose(nextCommunicationPoint, S->stopTime)) {
        logError(S, "At communication point %.16g a step size of %.16g was requested but stop time is %.16g.",
            currentCommunicationPoint, communicationStepSize, S->stopTime);
        CALL(Error);
    }

    while (true) {

        const double nextSolverStepTime = S->time + FIXED_SOLVER_STEP;

        if (nextSolverStepTime > nextCommunicationPoint && !isClose(nextSolverStepTime, nextCommunicationPoint)) {
            break;  // next communcation point reached
        }

        bool stateEvent, timeEvent;

        CALL(doFixedStep(S, &stateEvent, &timeEvent));

#ifdef EVENT_UPDATE
        if (stateEvent || timeEvent) {
            CALL(eventUpdate(S));
        }
#endif

        if (S->terminateSimulation) {
            status = Discard;
            goto TERMINATE;
        }
    }

    S->nextCommunicationPoint = nextCommunicationPoint;

    END_FUNCTION();
}

/* Inquire slave status */
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

fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status *value) {

    UNUSED(value);

    BEGIN_FUNCTION(GetStatus);

    CALL(getStatus("fmi2GetStatus", S, s));

    END_FUNCTION();
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real *value) {

    BEGIN_FUNCTION(GetRealStatus);

    if (s == fmi2LastSuccessfulTime) {
        *value = S->time;
        goto TERMINATE;
    }

    CALL(getStatus("fmi2GetRealStatus", c, s));

    END_FUNCTION();
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer *value) {

    UNUSED(value);

    BEGIN_FUNCTION(GetIntegerStatus);

    CALL(getStatus("fmi2GetIntegerStatus", c, s));

    END_FUNCTION();
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean *value) {

    BEGIN_FUNCTION(GetBooleanStatus);

    if (s == fmi2Terminated) {
        *value = S->terminateSimulation;
        goto TERMINATE;
    }

    CALL(getStatus("fmi2GetBooleanStatus", c, s));

    END_FUNCTION();
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String *value) {

    UNUSED(value);

    BEGIN_FUNCTION(GetStringStatus);

    CALL(getStatus("fmi2GetStringStatus", c, s));

    END_FUNCTION();
}

// ---------------------------------------------------------------------------
// Functions for FMI2 for Model Exchange
// ---------------------------------------------------------------------------
/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) {

    BEGIN_FUNCTION(EnterEventMode);

    S->state = EventMode;

    END_FUNCTION();
}

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *eventInfo) {

    BEGIN_FUNCTION(NewDiscreteStates);

#ifdef EVENT_UPDATE
    CALL(eventUpdate(S));
#endif

    eventInfo->newDiscreteStatesNeeded           = S->newDiscreteStatesNeeded;
    eventInfo->terminateSimulation               = S->terminateSimulation;
    eventInfo->nominalsOfContinuousStatesChanged = S->nominalsOfContinuousStatesChanged;
    eventInfo->valuesOfContinuousStatesChanged   = S->valuesOfContinuousStatesChanged;
    eventInfo->nextEventTimeDefined              = S->nextEventTimeDefined;
    eventInfo->nextEventTime                     = S->nextEventTime;

    END_FUNCTION();
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {

    BEGIN_FUNCTION(EnterContinuousTimeMode);

    S->state = ContinuousTimeMode;

    END_FUNCTION();
}

fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
                                     fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {

    UNUSED(noSetFMUStatePriorToCurrentPoint);

    BEGIN_FUNCTION(CompletedIntegratorStep);

    if (nullPointer(S, "fmi2CompletedIntegratorStep", "enterEventMode", enterEventMode))
        CALL(Error);

    if (nullPointer(S, "fmi2CompletedIntegratorStep", "terminateSimulation", terminateSimulation))
        CALL(Error);

    *enterEventMode = fmi2False;
    *terminateSimulation = fmi2False;

    END_FUNCTION();
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {

    BEGIN_FUNCTION(SetTime);

    S->time = time;

    END_FUNCTION();
}

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){

    BEGIN_FUNCTION(SetContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi2SetContinuousStates", "nx", nx, getNumberOfContinuousStates(S)))
        CALL(Error);

    if (nullPointer(S, "fmi2SetContinuousStates", "x[]", x))
        CALL(Error);

    CALL(setContinuousStates(S, x, nx));
#else
    UNUSED(x);

    if (invalidNumber(S, "fmi2SetContinuousStates", "nx", nx, 0))
        CALL(Error);
#endif

    END_FUNCTION();
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {

    BEGIN_FUNCTION(GetDerivatives);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi2GetDerivatives", "nx", nx, getNumberOfContinuousStates(S)))
        CALL(Error);

    if (nullPointer(S, "fmi2GetDerivatives", "derivatives[]", derivatives))
        CALL(Error);

    CALL(getDerivatives(S, derivatives, nx));
#else
    UNUSED(derivatives);

    if (invalidNumber(S, "fmi2GetDerivatives", "nx", nx, 0))
        CALL(Error);
#endif

    END_FUNCTION();
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {

    BEGIN_FUNCTION(GetEventIndicators);

#ifdef HAS_EVENT_INDICATORS
    if (invalidNumber(S, "fmi2GetEventIndicators", "ni", ni, getNumberOfEventIndicators(S)))
        CALL(Error);

    CALL(getEventIndicators(S, eventIndicators, ni));
#else
    UNUSED(eventIndicators);

    if (invalidNumber(S, "fmi2GetEventIndicators", "ni", ni, 0))
        CALL(Error);
#endif

    END_FUNCTION();
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real states[], size_t nx) {

    BEGIN_FUNCTION(GetContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi2GetContinuousStates", "nx", nx, getNumberOfContinuousStates(S)))
        CALL(Error);

    if (nullPointer(S, "fmi2GetContinuousStates", "states[]", states))
        CALL(Error);

    CALL(getContinuousStates(S, states, nx));
#else
    UNUSED(states);

    if (invalidNumber(S, "fmi2GetContinuousStates", "nx", nx, 0))
        CALL(Error);
#endif

    END_FUNCTION();
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {

    BEGIN_FUNCTION(GetNominalsOfContinuousStates);

#ifdef HAS_CONTINUOUS_STATES
    if (invalidNumber(S, "fmi2GetNominalContinuousStates", "nx", nx, getNumberOfContinuousStates(S)))
        CALL(Error);

    if (nullPointer(S, "fmi2GetNominalContinuousStates", "x_nominal[]", x_nominal))
        CALL(Error);

    for (size_t i = 0; i < nx; i++) {
        x_nominal[i] = 1;
    }
#else
    UNUSED(x_nominal);

    if (invalidNumber(S, "fmi2GetNominalContinuousStates", "nx", nx, 0))
        CALL(Error);
#endif

    END_FUNCTION();
}
