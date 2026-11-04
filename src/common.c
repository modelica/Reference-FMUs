#include <stdlib.h>  // for calloc(), free()
#include <float.h>   // for DBL_EPSILON
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "model_instance.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifdef CALL
#undef CALL
#endif

#define CALL(f) do { const Status status = f; if (status != OK) return status; } while (false)


static bool resourceLocationEndsWithFileSeparator(const char *resourceLocation) {

    if (!resourceLocation) {
        return true;
    }

    const size_t len = strlen(resourceLocation);

    if (len == 0) {
        return false;
    }

#ifdef _WIN32
    return resourceLocation[len - 1] == '\\' || resourceLocation[len - 1] == '/';
#else
    return resourceLocation[len - 1] == '/';
#endif
}

static ModelInstance *createModelInstanceImpl(
    loggerType cbLogger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    InterfaceType interfaceType) {

    ModelInstance *comp = NULL;

    if (!instanceName || strlen(instanceName) == 0) {
        if (cbLogger) {
#if FMI_VERSION < 3
            cbLogger(componentEnvironment, "?", Error, "error", "Missing instance name.");
#else
            cbLogger(componentEnvironment, Error, "error", "Missing instance name.");
#endif
        }
        return NULL;
    }

    if (!instantiationToken || strlen(instantiationToken) == 0) {
        if (cbLogger) {
#if FMI_VERSION < 3
            cbLogger(componentEnvironment, instanceName, Error, "error", "Missing GUID.");
#else
            cbLogger(componentEnvironment, Error, "error", "Missing instantiationToken.");
#endif
        }
        return NULL;
    }


    if (strcmp(instantiationToken, INSTANTIATION_TOKEN)) {
        if (cbLogger) {
#if FMI_VERSION < 3
            cbLogger(componentEnvironment, instanceName, Error, "error", "Wrong GUID.");
#else
            cbLogger(componentEnvironment, Error, "error", "Wrong instantiationToken.");
#endif
        }
        return NULL;
    }
    if (resourceLocation && !resourceLocationEndsWithFileSeparator(resourceLocation)) {
        if (cbLogger) {
#if FMI_VERSION < 3
            cbLogger(componentEnvironment, instanceName, Error, "error",
                "resourceLocation must end with a file separator.");
#else
            cbLogger(componentEnvironment, Error, "error",
                "resourceLocation must end with a file separator.");
#endif
        }
        return NULL;
    }

    comp = (ModelInstance *)calloc(1, sizeof(ModelInstance));

    if (comp) {
        comp->componentEnvironment = componentEnvironment;
        comp->logger               = cbLogger;
        comp->intermediateUpdate   = intermediateUpdate;
        comp->lockPreemption        = NULL;
        comp->unlockPreemption      = NULL;
        comp->instanceName         = strdup(instanceName);
        comp->resourceLocation     = resourceLocation ? strdup(resourceLocation) : NULL;
        comp->status               = OK;
        comp->logEvents            = loggingOn;
        comp->logErrors            = true; // always log errors
        comp->nSteps               = 0;
        comp->earlyReturnAllowed   = false;
        comp->eventModeUsed        = false;
    }

    if (!comp || !comp->instanceName) {
        logError(comp, "Out of memory.");
        return NULL;
    }

    comp->time                              = 0.0;  // overwrite in fmi*SetupExperiment, fmi*SetTime
    comp->type                              = interfaceType;

    comp->state                             = Instantiated;

    //comp->newDiscreteStatesNeeded           = false;
    //comp->terminateSimulation               = false;
    //comp->nominalsOfContinuousStatesChanged = false;
    //comp->valuesOfContinuousStatesChanged   = false;
    //comp->nextEventTimeDefined              = false;
    //comp->nextEventTime                     = 0;

    setStartValues(comp);

    comp->isDirtyValues = true;

    return comp;
}

ModelInstance *createModelExchangeInstance(
    loggerType logger,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn) {

    return createModelInstanceImpl(
        logger,
        NULL,
        componentEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        ModelExchange);
}

ModelInstance *createCoSimulationInstance(
    loggerType logger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    bool earlyReturnAllowed,
    bool eventModeUsed) {

#ifndef EVENT_UPDATE
    if (eventModeUsed) {
        if (logger) {
#if FMI_VERSION < 3
            logger(componentEnvironment, instanceName, Error, "error", "Event Mode is not supported.");
#else
            logger(componentEnvironment, Error, "error", "Event Mode is not supported.");
#endif
        }
        return NULL;
    }
#endif

    ModelInstance *comp = createModelInstanceImpl(
        logger,
        intermediateUpdate,
        componentEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        CoSimulation);

    if (comp) {
        comp->earlyReturnAllowed = earlyReturnAllowed;
        comp->eventModeUsed      = eventModeUsed;
    }

    return comp;
}

ModelInstance *createScheduledExecutionInstance(
    loggerType logger,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    clockUpdateType clockUpdate,
    lockPreemptionType lockPreemption,
    unlockPreemptionType unlockPreemption) {

    ModelInstance *comp = createModelInstanceImpl(
        logger,
        NULL,
        componentEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        ScheduledExecution);

    if (comp) {
        comp->clockUpdate      = clockUpdate;
        comp->lockPreemption   = lockPreemption;
        comp->unlockPreemption = unlockPreemption;
    }

    return comp;
}

ModelInstance *createModelInstance(
    loggerType cbLogger,
    intermediateUpdateType intermediateUpdate,
    void *componentEnvironment,
    const char *instanceName,
    const char *instantiationToken,
    const char *resourceLocation,
    bool loggingOn,
    InterfaceType interfaceType) {

    return createModelInstanceImpl(
        cbLogger,
        intermediateUpdate,
        componentEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        interfaceType);
}

void freeModelInstance(ModelInstance *comp) {

    if (!comp) return;

    if (comp->instanceName) free((void*)comp->instanceName);

    if (comp->resourceLocation) free((void*)comp->resourceLocation);

    free(comp);
}

static Status s_reallocate(ModelInstance* comp, void** memory, size_t size) {

    if (size == 0) {
        if (*memory) {
            free(*memory);
        }
        *memory = NULL;
        return OK;
    }

    void* temp = realloc(*memory, size);

    if (!temp) {
        logError(comp, "Failed to allocate memory.");
        return Error;
    }

    *memory = temp;

    return OK;
}

Status configurate(ModelInstance* comp) {

    (void)comp;

#ifdef MAX_EVENT_INDICATORS
    CALL(getNumberOfEventIndicators(comp, &comp->nz));
    CALL(getEventIndicators(comp, comp->prez, comp->nz));
#endif

#if MAX_CONTINUOUS_STATES > 0
    CALL(getNumberOfContinuousStates(comp, &comp->nx));
#endif

    return OK;
}

Status reset(ModelInstance* comp) {
    comp->state = Instantiated;
    comp->startTime = 0.0;
    comp->time = 0.0;
    comp->nSteps = 0;
    comp->status = OK;
    comp->isDirtyValues = true;

    return setStartValues(comp);
}

#define EPSILON (1.0e-5)

static double fmiAbs(double v) {
    return v >= 0 ? v : -v;
}

static double fmiMax(double a, double b) {
    return (a < b) ? b : a;
}

bool isClose(double a, double b) {

    if (fmiAbs(a - b) <= EPSILON) {
        return true;
    }

    return fmiAbs(a - b) <= EPSILON * fmiMax(fmiAbs(a), fmiAbs(b));
}

bool invalidNumber(ModelInstance *comp, const char *f, const char *arg, size_t actual, size_t expected) {

    if (actual != expected) {
        comp->state = Terminated;
        logError(comp, "%s: Invalid argument %s = %d. Expected %d.", f, arg, actual, expected);
        return true;
    }

    return false;
}

bool invalidState(ModelInstance *comp, const char *f, int statesExpected) {

    UNUSED(f);
    UNUSED(statesExpected);

    if (!comp) {
        return true;
    }

    // TODO: add missing states and check state
    return false;

//    if (!(comp->state & statesExpected)) {
//        comp->state = modelError;
//        logError(comp, "%s: Illegal call sequence.", f);
//        return true;
//    }
//
//    return false;
}

bool nullPointer(ModelInstance* comp, const char *f, const char *arg, const void *p) {

    if (!p) {
        comp->state = Terminated;
        logError(comp, "%s: Invalid argument %s = NULL.", f, arg);
        return true;
    }

    return false;
}

Status setupExperiment(
    ModelInstance* comp,
    bool toleranceDefined,
    double tolerance,
    double startTime,
    bool stopTimeDefined,
    double stopTime
) {
    // TODO: check comp != NULL
    if (comp->state != Instantiated) {
        logError(comp, "setupExperiment() may only be called in Instantiated Mode");
        return Discard;
    }

    // TODO: check values
    comp->startTime = startTime;
    comp->stopTimeDefined = stopTimeDefined;
    comp->stopTime = stopTime;
    comp->nextCommunicationPoint = startTime;

    return OK;
}

Status enterConfigurationMode(ModelInstance* comp) {
    switch (comp->type) {
    case ModelExchange:
        switch (comp->state) {
        case Instantiated:
            comp->state = ConfigurationMode;
            return OK;
        case EventMode:
            comp->state = ReconfigurationMode;
            return OK;
        default:
            logError(comp, "enterConfigurationMode() may only be called in Instantiated and Event Mode");
        }
        break;
    case CoSimulation:
        switch (comp->state) {
        case Instantiated:
            comp->state = ConfigurationMode;
            return OK;
        case StepMode:
            comp->state = ReconfigurationMode;
            return OK;
        default:
            logError(comp, "enterConfigurationMode() may only be called in Instantiated and Step Mode");
        }
        break;
    case ScheduledExecution:
        switch (comp->state) {
        case Instantiated:
            comp->state = ConfigurationMode;
            return OK;
        case ClockActivationMode:
            comp->state = ReconfigurationMode;
            return OK;
        default:
            logError(comp, "enterConfigurationMode() may only be called in Instantiated and Clock Activation Mode");
        }
        break;
    }

    return Discard;
}

Status exitConfigurationMode(ModelInstance* comp) {

    switch (comp->state) {
    case ConfigurationMode:
    case ReconfigurationMode:
        break;
    default:
        logError(comp, "exitConfigurationMode() may only be called in Configuration and Reconfiguration Mode");
        return Discard;
    }

    const Status status = configurate(comp);

    if (status == OK) {
        switch (comp->state) {
        case ConfigurationMode:
            comp->state = Instantiated;
            break;
        case ReconfigurationMode:
            switch (comp->type) {
            case ModelExchange:
                comp->state = EventMode;
                break;
            case CoSimulation:
                comp->state = StepMode;
                break;
            case ScheduledExecution:
                comp->state = ClockActivationMode;
                break;
            }
            break;
        }
    }

    return status;
}

Status enterInitializationMode(ModelInstance* comp) {
    if (comp->state != Instantiated) {
        logError(comp, "enterInitializationMode() may only be called in Instantiated Mode");
        return Discard;
    }

    comp->state = InitializationMode;

    return OK;
}

Status exitInitializationMode(ModelInstance* comp) {
    if (comp->state != InitializationMode) {
        logError(comp, "exitInitializationMode() may only be called in Initialization Mode");
        return Discard;
    }

    const Status status = configurate(comp);

    if (status == OK) {
        switch (comp->type) {
            case CoSimulation:
                comp->state = comp->eventModeUsed ? EventMode : StepMode;
                break;
            case ModelExchange:
                comp->state = EventMode;
                break;
            case ScheduledExecution:
                comp->state = ClockActivationMode;
                break;
        }
    }
    
    return status;
}

Status enterEventMode(ModelInstance* comp) {
    switch (comp->type) {
        case CoSimulation: 
            if (comp->state != StepMode) {
                logError(comp, "enterEventMode() may only be called in Step Mode");
                return Discard;
            } else if (!comp->eventModeUsed) {
                logError(comp, "enterEventMode() may only be called if eventModeUsed is true");
                return Discard;
            }
            break;
        case ModelExchange: 
            if (comp->state != ContinuousTimeMode) {
                logError(comp, "enterEventMode() may only be called in Continuous Time Mode");
                return Discard;
            }
            break;
        case ScheduledExecution: 
            logError(comp, "enterEventMode() may not be called in Scheduled Execution");
            return Discard;
    }

    comp->state = EventMode;

    return OK;
}

Status enterContinuousTimeMode(ModelInstance* comp) {
    switch (comp->type) {
        case ModelExchange:
            if (comp->state != EventMode) {
                logError(comp, "enterContinuousTimeMode() may only be called in Event Mode");
                return Discard;
            }
            comp->state = ContinuousTimeMode;
            return OK;
        default:
            logError(comp, "enterContinuousTimeMode() is only allowed in Model Exchange");
            return Discard;
    }
}

Status terminate(ModelInstance* comp) {
    switch (comp->type) {
        case CoSimulation: {
            switch (comp->state) {
            case StepMode:
            case EventMode:
                break;
            default:
                logError(comp, "terminate() may only be called in Step Mode and Event Mode");
                return Discard;
            }
            break;
        }
        case ModelExchange: {
            switch (comp->state) {
            case ContinuousTimeMode:
            case EventMode:
                break;
            default:
                logError(comp, "terminate() may only be called in Continuous Time Mode and Event Mode");
                return Discard;
            }
            break;
        }
        case ScheduledExecution: {
            switch (comp->state) {
            case ClockActivationMode:
                break;
            default:
                logError(comp, "terminate() may only be called in Clock Activation Mode");
                return Discard;
            }
            break;
        }
    }

    comp->state = Terminated;

    return OK;
}

Status completedIntegratorStep(ModelInstance* comp,
    bool  noSetFMUStatePriorToCurrentPoint,
    bool* enterEventMode,
    bool* terminateSimulation) {
    return OK;
}

Status setTime(ModelInstance* comp, double time) {
    comp->time = time;
    return OK;
}

Status enterStepMode(ModelInstance* comp) {
    switch (comp->type) {
    case CoSimulation: {
        if (comp->state != EventMode) {
            logError(comp, "enterStepMode() may only be called in Event Mode");
            return Discard;
        }
        break;
    }
    default:
        logError(comp, "enterEventMode() may not be called in Scheduled Execution");
        return Discard;
    }

    comp->state = StepMode;

    return OK;
}

Status setDebugLogging(ModelInstance *comp, bool loggingOn, size_t nCategories, const char * const categories[]) {

    if (nCategories > 0) {

        if (categories == NULL) {
            logError(comp, "Argument categories must not be NULL.");
            return Error;
        }

        for (size_t i = 0; i < nCategories; i++) {

            if (categories[i] == NULL) {
                logError(comp, "Argument categories[%zu] must not be NULL.", i);
                return Error;
            } else if (strcmp(categories[i], "logEvents") == 0) {
                comp->logEvents = loggingOn;
            } else if (strcmp(categories[i], "logStatusError") == 0) {
                comp->logErrors = loggingOn;
            } else {
                logError(comp, "Log categories[%zu] must be one of \"logEvents\" or \"logStatusError\" but was \"%s\".", i, categories[i]);
                return Error;
            }
        }

    } else {

        comp->logEvents = loggingOn;
        comp->logErrors = loggingOn;

    }

    return OK;
}

static void logMessage(ModelInstance *comp, int status, const char *category, const char *message, va_list args) {

    if (!comp->logger) {
        return;
    }

    va_list args1;
    int len = 0;
    char *buf = "";

    va_copy(args1, args);
    len = vsnprintf(buf, len, message, args1);
    va_end(args1);

    if (len < 0) {
        return;
    }

    va_copy(args1, args);
    buf = (char *)calloc(len + 1, sizeof(char));
    len = vsnprintf(buf, len + 1, message, args);
    va_end(args1);

    if (len >= 0) {
        // no need to distinguish between FMI versions since we're not using variadic arguments
#if FMI_VERSION < 3
        comp->logger(comp->componentEnvironment, comp->instanceName, status, category, buf);
#else
        comp->logger(comp->componentEnvironment, status, category, buf);
#endif
    }

    free(buf);
}

void logEvent(ModelInstance *comp, const char *message, ...) {

    if (!comp || !comp->logEvents) return;

    va_list args;
    va_start(args, message);
    logMessage(comp, OK, "logEvents", message, args);
    va_end(args);
}

void logError(ModelInstance *comp, const char *message, ...) {

    if (!comp || !comp->logErrors) return;

    va_list args;
    va_start(args, message);
    logMessage(comp, Error, "logStatusError", message, args);
    va_end(args);
}

#define GET_NOT_ALLOWED(t) do { \
    UNUSED(vr); \
    UNUSED(values); \
    UNUSED(nValues); \
    UNUSED(index); \
    logError(comp, "Getting " t " is not allowed.");\
    return Error; \
} while (false)

#ifndef GET_FLOAT32
Status getFloat32(ModelInstance* comp, ValueReference vr, float values[], size_t nValues, size_t* index) {
    GET_NOT_ALLOWED("Float32");
}
#endif

#ifndef GET_INT8
Status getInt8(ModelInstance* comp, ValueReference vr, int8_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int8");
}
#endif

#ifndef GET_UINT8
Status getUInt8(ModelInstance* comp, ValueReference vr, uint8_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt8");
}
#endif

#ifndef GET_INT16
Status getInt16(ModelInstance* comp, ValueReference vr, int16_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int16");

}
#endif

#ifndef GET_UINT16
Status getUInt16(ModelInstance* comp, ValueReference vr, uint16_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt16");
}
#endif

#ifndef GET_INT32
Status getInt32(ModelInstance* comp, ValueReference vr, int32_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int32");
}
#endif

#ifndef GET_UINT32
Status getUInt32(ModelInstance* comp, ValueReference vr, uint32_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt32");
}
#endif

#ifndef GET_INT64
Status getInt64(ModelInstance* comp, ValueReference vr, int64_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int64");
}
#endif

#ifndef GET_UINT64
Status getUInt64(ModelInstance* comp, ValueReference vr, uint64_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt64");
}
#endif

#ifndef GET_BOOLEAN
Status getBoolean(ModelInstance* comp, ValueReference vr, bool values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Boolean");
}
#endif

#ifndef GET_STRING
Status getString(ModelInstance* comp, ValueReference vr, const char* values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("String");
}
#endif

#ifndef GET_BINARY
Status getBinary(ModelInstance* comp, ValueReference vr, size_t sizes[], const char* values[], size_t nValues, size_t *index) {
    UNUSED(sizes);
    GET_NOT_ALLOWED("Binary");
}
#endif

#define SET_NOT_ALLOWED(t) do { \
    UNUSED(vr); \
    UNUSED(values); \
    UNUSED(nValues); \
    UNUSED(index); \
    logError(comp, "Setting " t " is not allowed.");\
    return Error; \
} while (false)

#ifndef SET_FLOAT32
Status setFloat32(ModelInstance* comp, ValueReference vr, const float values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Float32");
}
#endif

#ifndef SET_FLOAT64
Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Float64");
}
#endif

#ifndef SET_INT8
Status setInt8(ModelInstance* comp, ValueReference vr, const int8_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int8");
}
#endif

#ifndef SET_UINT8
Status setUInt8(ModelInstance* comp, ValueReference vr, const uint8_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt8");
}
#endif

#ifndef SET_INT16
Status setInt16(ModelInstance* comp, ValueReference vr, const int16_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int16");
}
#endif

#ifndef SET_UINT16
Status setUInt16(ModelInstance* comp, ValueReference vr, const uint16_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt16");
}
#endif

#ifndef SET_INT32
Status setInt32(ModelInstance* comp, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int32");
}
#endif

#ifndef SET_UINT32
Status setUInt32(ModelInstance* comp, ValueReference vr, const uint32_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt32");
}
#endif

#ifndef SET_INT64
Status setInt64(ModelInstance* comp, ValueReference vr, const int64_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int64");
}
#endif

#ifndef SET_UINT64
Status setUInt64(ModelInstance* comp, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt64");
}
#endif

#ifndef SET_BOOLEAN
Status setBoolean(ModelInstance* comp, ValueReference vr, const bool values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Boolean");
}
#endif

#ifndef SET_STRING
Status setString(ModelInstance* comp, ValueReference vr, const char *const values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("String");
}
#endif

#ifndef SET_BINARY
Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char *const values[], size_t nValues, size_t* index) {
    UNUSED(size);
    SET_NOT_ALLOWED("Binary");
}
#endif

#ifndef MAX_CONTINUOUS_STATES
size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return 0;
}

Status getContinuousStates(ModelInstance* comp, double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    UNUSED(x);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance* comp, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    UNUSED(nominals);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}

Status setContinuousStates(ModelInstance* comp, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    UNUSED(x);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}

Status getDerivatives(ModelInstance* comp, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    UNUSED(dx);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}
#endif

#ifndef MAX_EVENT_INDICATORS
size_t getNumberOfEventIndicators(ModelInstance* comp) {
    UNUSED(comp);
    return 0;
}

Status getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    ASSERT_NOT_NULL2(comp);
    UNUSED(z);
    ASSERT_SIZE_T(nz, 0);
    return OK;
}
#endif

#ifndef GET_PARTIAL_DERIVATIVE
Status getPartialDerivative(ModelInstance* comp, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double* partialDerivative) {
    UNUSED(comp);
    UNUSED(unknown);
    UNUSED(unknownIndex);
    UNUSED(known);
    UNUSED(knownIndex);
    UNUSED(partialDerivative);
    logError(comp, "Directional derivatives are not supported.");
    return Error;
}
#endif

#ifndef ACTIVATE_CLOCK
Status activateClock(ModelInstance* comp, ValueReference vr) {
    UNUSED(comp);
    UNUSED(vr);
    return Error;
}
#endif

#ifndef GET_CLOCK
Status getClock(ModelInstance* comp, ValueReference vr, bool* value) {
    UNUSED(comp);
    UNUSED(vr);
    UNUSED(value);
    return Error;
}
#endif

#ifndef GET_INTERVAL
Status getInterval(ModelInstance* comp, ValueReference vr, double* interval, int* qualifier) {
    UNUSED(comp);
    UNUSED(vr);
    UNUSED(interval);
    UNUSED(qualifier);
    return Error;
}
#endif

#ifndef ACTIVATE_MODEL_PARTITION
Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime) {
    UNUSED(comp);
    UNUSED(vr);
    UNUSED(activationTime);
    return Error;
}
#endif

Status getFMUState(ModelInstance* comp, void** FMUState) {

    CALL(s_reallocate(comp, FMUState, sizeof(ModelInstance)));

    memcpy(*FMUState, comp, sizeof(ModelInstance));

    return OK;
}

Status setFMUState(ModelInstance* comp, void* FMUState) {

    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(FMUState);

    ModelInstance* s = (ModelInstance*)FMUState;

    comp->startTime = s->startTime;
    comp->stopTime = s->stopTime;
    comp->time = s->time;
    // instanceName
    // type
    // resourceLocation

    comp->status = s->status;

    // logger
    // intermediateUpdate
    // clockUpdate

    // lockPreemption
    // unlockPreemption

    // logEvents
    // logErrors

    // componentEnvironment
    comp->state = s->state;

    //comp->newDiscreteStatesNeeded = s->newDiscreteStatesNeeded;
    //comp->terminateSimulation = s->terminateSimulation;
    //comp->nominalsOfContinuousStatesChanged = s->nominalsOfContinuousStatesChanged;
    //comp->valuesOfContinuousStatesChanged = s->valuesOfContinuousStatesChanged;
    comp->nextEventTimeDefined = s->nextEventTimeDefined;
    comp->nextEventTime = s->nextEventTime;
    comp->clocksTicked = s->clocksTicked;

    comp->isDirtyValues = s->isDirtyValues;

    memcpy(& comp->modelData, & s->modelData, sizeof(ModelData));

    comp->nSteps = s->nSteps;

    comp->earlyReturnAllowed = s->earlyReturnAllowed;
    comp->eventModeUsed = s->eventModeUsed;
    comp->nextCommunicationPoint = s->nextCommunicationPoint;

#if MAX_EVENT_INDICATORS > 0
    comp->nz = s->nz;
    memcpy(comp->z, s->z, s->nz * sizeof(double));
    memcpy(comp->prez, s->prez, s->nz * sizeof(double));
#endif

#if MAX_CONTINUOUS_STATES > 0
    comp->nx = s->nx;
    memcpy(comp->x, s->x, s->nx * sizeof(double));
    memcpy(comp->dx, s->dx, s->nx * sizeof(double));
#endif

    comp->nSteps = s->nSteps;

    return OK;
}

Status serializeFMUState(
    ModelInstance* comp,
    void*  FMUState,
    uint8_t serializedState[],
    size_t size) {
    memcpy(serializedState, FMUState, sizeof(ModelInstance));
    return OK;
}

Status deserializeFMUState(
    ModelInstance* comp,
    const uint8_t serializedState[],
    size_t size,
    void** FMUState
) {
    if (*FMUState == NULL) {
        *FMUState = calloc(1, sizeof(ModelInstance));
        if (*FMUState == NULL) {
            printf("Failed to allocate memory for FMUState.\n");
            return Discard;
        }
    }

    memcpy(*FMUState, serializedState, sizeof(ModelInstance));

    return OK;
}
