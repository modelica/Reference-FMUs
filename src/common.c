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

    ModelInstance *instance = NULL;

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

    instance = (ModelInstance *)calloc(1, sizeof(ModelInstance));

    if (instance) {
        instance->componentEnvironment = componentEnvironment;
        instance->logger               = cbLogger;
        instance->intermediateUpdate   = intermediateUpdate;
        instance->lockPreemption       = NULL;
        instance->unlockPreemption     = NULL;
        instance->instanceName         = strdup(instanceName);
        instance->resourceLocation     = resourceLocation ? strdup(resourceLocation) : NULL;
        instance->status               = OK;
        instance->logEvents            = loggingOn;
        instance->logErrors            = true;
        instance->nSteps               = 0;
        instance->earlyReturnAllowed   = false;
        instance->eventModeUsed        = false;
    }

    if (!instance || !instance->instanceName) {
        logError(instance, "Out of memory.");
        return NULL;
    }

    instance->time = 0.0;
    instance->type = interfaceType;
    instance->state = Instantiated;

    setStartValues(instance);

    instance->isDirtyValues = true;

    return instance;
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

    ModelInstance *instance = createModelInstanceImpl(
        logger,
        intermediateUpdate,
        componentEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        CoSimulation);

    if (instance) {
        instance->earlyReturnAllowed = earlyReturnAllowed;
        instance->eventModeUsed      = eventModeUsed;
    }

    return instance;
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

    ModelInstance *instance = createModelInstanceImpl(
        logger,
        NULL,
        componentEnvironment,
        instanceName,
        instantiationToken,
        resourceLocation,
        loggingOn,
        ScheduledExecution);

    if (instance) {
        instance->clockUpdate      = clockUpdate;
        instance->lockPreemption   = lockPreemption;
        instance->unlockPreemption = unlockPreemption;
    }

    return instance;
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

void freeModelInstance(ModelInstance *instance) {

    if (!instance) return;

    if (instance->instanceName) free((void*)instance->instanceName);

    if (instance->resourceLocation) free((void*)instance->resourceLocation);

    free(instance);
}

static Status s_reallocate(ModelInstance *instance, void** memory, size_t size) {

    if (size == 0) {
        if (*memory) {
            free(*memory);
        }
        *memory = NULL;
        return OK;
    }

    void* temp = realloc(*memory, size);

    if (!temp) {
        logError(instance, "Failed to allocate memory.");
        return Error;
    }

    *memory = temp;

    return OK;
}

Status reset(ModelInstance *instance) {
    instance->state = Instantiated;
    instance->startTime = 0.0;
    instance->time = 0.0;
    instance->nSteps = 0;
    instance->status = OK;
    instance->isDirtyValues = true;

    return setStartValues(instance);
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

bool invalidNumber(ModelInstance *instance, const char *f, const char *arg, size_t actual, size_t expected) {

    if (actual != expected) {
        instance->state = Terminated;
        logError(instance, "%s: Invalid argument %s = %d. Expected %d.", f, arg, actual, expected);
        return true;
    }

    return false;
}

bool invalidState(ModelInstance *instance, const char *f, int statesExpected) {

    UNUSED(f);
    UNUSED(statesExpected);

    if (!instance) {
        return true;
    }

    // TODO: add missing states and check state
    return false;

//    if (!(instance->state & statesExpected)) {
//        instance->state = modelError;
//        logError(instance, "%s: Illegal call sequence.", f);
//        return true;
//    }
//
//    return false;
}

bool nullPointer(ModelInstance *instance, const char *f, const char *arg, const void *p) {

    if (!p) {
        instance->state = Terminated;
        logError(instance, "%s: Invalid argument %s = NULL.", f, arg);
        return true;
    }

    return false;
}

Status setupExperiment(
    ModelInstance *instance,
    bool toleranceDefined,
    double tolerance,
    double startTime,
    bool stopTimeDefined,
    double stopTime
) {

    // TODO: check instance != NULL
    if (instance->state != Instantiated) {
        logError(instance, "setupExperiment() may only be called in Instantiated Mode");
        return Discard;
    }

    if (toleranceDefined) {
        logError(instance, "Relative tolerance is not supported.");
        return Error;
    }

    UNUSED(tolerance);

    // TODO: check values
    instance->startTime = startTime;
    instance->stopTimeDefined = stopTimeDefined;
    instance->stopTime = stopTime;
    instance->nextCommunicationPoint = startTime;

    return OK;
}

Status enterConfigurationMode(ModelInstance *instance) {
#ifdef CONFIGURATE
    switch (instance->type) {
    case ModelExchange:
        switch (instance->state) {
        case Instantiated:
            instance->state = ConfigurationMode;
            return OK;
        case EventMode:
            instance->state = ReconfigurationMode;
            return OK;
        default:
            logError(instance, "enterConfigurationMode() may only be called in Instantiated and Event Mode");
        }
        break;
    case CoSimulation:
        switch (instance->state) {
        case Instantiated:
            instance->state = ConfigurationMode;
            return OK;
        case StepMode:
            instance->state = ReconfigurationMode;
            return OK;
        default:
            logError(instance, "enterConfigurationMode() may only be called in Instantiated and Step Mode");
        }
        break;
    case ScheduledExecution:
        switch (instance->state) {
        case Instantiated:
            instance->state = ConfigurationMode;
            return OK;
        case ClockActivationMode:
            instance->state = ReconfigurationMode;
            return OK;
        default:
            logError(instance, "enterConfigurationMode() may only be called in Instantiated and Clock Activation Mode");
        }
        break;
    }

    return Discard;
#else
    logError(instance, "enterConfigurationMode() is not supported");
    return Discard;
#endif
}

Status exitConfigurationMode(ModelInstance *instance) {
#ifdef CONFIGURATE
    switch (instance->state) {
    case ConfigurationMode:
    case ReconfigurationMode:
        break;
    default:
        logError(instance, "exitConfigurationMode() may only be called in Configuration and Reconfiguration Mode");
        return Discard;
    }

    const Status status = configurate(instance);

    if (status == OK) {
        switch (instance->state) {
        case ConfigurationMode:
            instance->state = Instantiated;
            break;
        case ReconfigurationMode:
            switch (instance->type) {
            case ModelExchange:
                instance->state = EventMode;
                break;
            case CoSimulation:
                instance->state = StepMode;
                break;
            case ScheduledExecution:
                instance->state = ClockActivationMode;
                break;
            }
            break;
        }
    }

    return status;
#else
    logError(instance, "exitConfigurationMode() is not supported");
    return Discard;
#endif
}

Status enterInitializationMode(ModelInstance *instance) {
    if (instance->state != Instantiated) {
        logError(instance, "enterInitializationMode() may only be called in Instantiated Mode");
        return Discard;
    }

    instance->state = InitializationMode;

    return OK;
}

Status exitInitializationMode(ModelInstance *instance) {
    if (instance->state != InitializationMode) {
        logError(instance, "exitInitializationMode() may only be called in Initialization Mode");
        return Discard;
    }

    switch (instance->type) {
        case CoSimulation:
#if defined(N_CONTINUOUS_STATES) || defined(CONFIGURATE)
            CALL(getNumberOfContinuousStates(instance, &instance->nx));
#endif
#if defined(N_EVENT_INDICATORS) || defined(CONFIGURATE)
            CALL(getNumberOfEventIndicators(instance, &instance->nz));
#endif
            instance->state = instance->eventModeUsed ? EventMode : StepMode;
            break;
        case ModelExchange:
            instance->state = EventMode;
            break;
        case ScheduledExecution:
            instance->state = ClockActivationMode;
            break;
    }
    
    return OK;
}

Status enterEventMode(ModelInstance *instance) {
    switch (instance->type) {
        case CoSimulation: 
            if (instance->state != StepMode) {
                logError(instance, "enterEventMode() may only be called in Step Mode");
                return Discard;
            } else if (!instance->eventModeUsed) {
                logError(instance, "enterEventMode() may only be called if eventModeUsed is true");
                return Discard;
            }
            break;
        case ModelExchange: 
            if (instance->state != ContinuousTimeMode) {
                logError(instance, "enterEventMode() may only be called in Continuous Time Mode");
                return Discard;
            }
            break;
        case ScheduledExecution: 
            logError(instance, "enterEventMode() may not be called in Scheduled Execution");
            return Discard;
    }

    instance->state = EventMode;

    return OK;
}

Status enterContinuousTimeMode(ModelInstance *instance) {
    switch (instance->type) {
        case ModelExchange:
            if (instance->state != EventMode) {
                logError(instance, "enterContinuousTimeMode() may only be called in Event Mode");
                return Discard;
            }
            instance->state = ContinuousTimeMode;
            return OK;
        default:
            logError(instance, "enterContinuousTimeMode() is only allowed in Model Exchange");
            return Discard;
    }
}

Status terminate(ModelInstance *instance) {
    switch (instance->type) {
        case CoSimulation: {
            switch (instance->state) {
            case StepMode:
            case EventMode:
                break;
            default:
                logError(instance, "terminate() may only be called in Step Mode and Event Mode");
                return Discard;
            }
            break;
        }
        case ModelExchange: {
            switch (instance->state) {
            case ContinuousTimeMode:
            case EventMode:
                break;
            default:
                logError(instance, "terminate() may only be called in Continuous Time Mode and Event Mode");
                return Discard;
            }
            break;
        }
        case ScheduledExecution: {
            switch (instance->state) {
            case ClockActivationMode:
                break;
            default:
                logError(instance, "terminate() may only be called in Clock Activation Mode");
                return Discard;
            }
            break;
        }
    }

    instance->state = Terminated;

    return OK;
}

Status completedIntegratorStep(ModelInstance *instance,
    bool  noSetFMUStatePriorToCurrentPoint,
    bool* enterEventMode,
    bool* terminateSimulation) {
    UNUSED(instance);
    UNUSED(noSetFMUStatePriorToCurrentPoint);
    UNUSED(enterEventMode);
    UNUSED(terminateSimulation);
    return OK;
}

Status setTime(ModelInstance *instance, double time) {
    instance->time = time;
    return OK;
}

Status enterStepMode(ModelInstance *instance) {
    switch (instance->type) {
    case CoSimulation: {
        if (instance->state != EventMode) {
            logError(instance, "enterStepMode() may only be called in Event Mode");
            return Discard;
        }
        break;
    }
    default:
        logError(instance, "enterEventMode() may not be called in Scheduled Execution");
        return Discard;
    }

    instance->state = StepMode;

    return OK;
}

Status setDebugLogging(ModelInstance *instance, bool loggingOn, size_t nCategories, const char * const categories[]) {

    if (nCategories > 0) {

        if (categories == NULL) {
            logError(instance, "Argument categories must not be NULL.");
            return Error;
        }

        for (size_t i = 0; i < nCategories; i++) {

            if (categories[i] == NULL) {
                logError(instance, "Argument categories[%zu] must not be NULL.", i);
                return Error;
            } else if (strcmp(categories[i], "logEvents") == 0) {
                instance->logEvents = loggingOn;
            } else if (strcmp(categories[i], "logStatusError") == 0) {
                instance->logErrors = loggingOn;
            } else {
                logError(instance, "Log categories[%zu] must be one of \"logEvents\" or \"logStatusError\" but was \"%s\".", i, categories[i]);
                return Error;
            }
        }

    } else {

        instance->logEvents = loggingOn;
        instance->logErrors = loggingOn;

    }

    return OK;
}

static void logMessage(ModelInstance *instance, int status, const char *category, const char *message, va_list args) {

    if (!instance->logger) {
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
        instance->logger(instance->componentEnvironment, instance->instanceName, status, category, buf);
#else
        instance->logger(instance->componentEnvironment, status, category, buf);
#endif
    }

    free(buf);
}

void logEvent(ModelInstance *instance, const char *message, ...) {

    if (!instance || !instance->logEvents) return;

    va_list args;
    va_start(args, message);
    logMessage(instance, OK, "logEvents", message, args);
    va_end(args);
}

void logError(ModelInstance *instance, const char *message, ...) {

    if (!instance || !instance->logErrors) return;

    va_list args;
    va_start(args, message);
    logMessage(instance, Error, "logStatusError", message, args);
    va_end(args);
}

#define GET_NOT_ALLOWED(t) do { \
    UNUSED(vr); \
    UNUSED(values); \
    UNUSED(nValues); \
    UNUSED(index); \
    logError(instance, "Getting " t " is not allowed.");\
    return Error; \
} while (false)

#ifndef GET_FLOAT32
Status getFloat32(ModelInstance *instance, ValueReference vr, float values[], size_t nValues, size_t* index) {
    GET_NOT_ALLOWED("Float32");
}
#endif

#ifndef GET_INT8
Status getInt8(ModelInstance *instance, ValueReference vr, int8_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int8");
}
#endif

#ifndef GET_UINT8
Status getUInt8(ModelInstance *instance, ValueReference vr, uint8_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt8");
}
#endif

#ifndef GET_INT16
Status getInt16(ModelInstance *instance, ValueReference vr, int16_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int16");

}
#endif

#ifndef GET_UINT16
Status getUInt16(ModelInstance *instance, ValueReference vr, uint16_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt16");
}
#endif

#ifndef GET_INT32
Status getInt32(ModelInstance *instance, ValueReference vr, int32_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int32");
}
#endif

#ifndef GET_UINT32
Status getUInt32(ModelInstance *instance, ValueReference vr, uint32_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt32");
}
#endif

#ifndef GET_INT64
Status getInt64(ModelInstance *instance, ValueReference vr, int64_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Int64");
}
#endif

#ifndef GET_UINT64
Status getUInt64(ModelInstance *instance, ValueReference vr, uint64_t values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("UInt64");
}
#endif

#ifndef GET_BOOLEAN
Status getBoolean(ModelInstance *instance, ValueReference vr, bool values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("Boolean");
}
#endif

#ifndef GET_STRING
Status getString(ModelInstance *instance, ValueReference vr, const char* values[], size_t nValues, size_t *index) {
    GET_NOT_ALLOWED("String");
}
#endif

#ifndef GET_BINARY
Status getBinary(ModelInstance *instance, ValueReference vr, size_t sizes[], const char* values[], size_t nValues, size_t *index) {
    UNUSED(sizes);
    GET_NOT_ALLOWED("Binary");
}
#endif

#define SET_NOT_ALLOWED(t) do { \
    UNUSED(vr); \
    UNUSED(values); \
    UNUSED(nValues); \
    UNUSED(index); \
    logError(instance, "Setting " t " is not allowed.");\
    return Error; \
} while (false)

#ifndef SET_FLOAT32
Status setFloat32(ModelInstance *instance, ValueReference vr, const float values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Float32");
}
#endif

#ifndef SET_FLOAT64
Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Float64");
}
#endif

#ifndef SET_INT8
Status setInt8(ModelInstance *instance, ValueReference vr, const int8_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int8");
}
#endif

#ifndef SET_UINT8
Status setUInt8(ModelInstance *instance, ValueReference vr, const uint8_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt8");
}
#endif

#ifndef SET_INT16
Status setInt16(ModelInstance *instance, ValueReference vr, const int16_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int16");
}
#endif

#ifndef SET_UINT16
Status setUInt16(ModelInstance *instance, ValueReference vr, const uint16_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt16");
}
#endif

#ifndef SET_INT32
Status setInt32(ModelInstance *instance, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int32");
}
#endif

#ifndef SET_UINT32
Status setUInt32(ModelInstance *instance, ValueReference vr, const uint32_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt32");
}
#endif

#ifndef SET_INT64
Status setInt64(ModelInstance *instance, ValueReference vr, const int64_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Int64");
}
#endif

#ifndef SET_UINT64
Status setUInt64(ModelInstance *instance, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("UInt64");
}
#endif

#ifndef SET_BOOLEAN
Status setBoolean(ModelInstance *instance, ValueReference vr, const bool values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("Boolean");
}
#endif

#ifndef SET_STRING
Status setString(ModelInstance *instance, ValueReference vr, const char *const values[], size_t nValues, size_t* index) {
    SET_NOT_ALLOWED("String");
}
#endif

#ifndef SET_BINARY
Status setBinary(ModelInstance *instance, ValueReference vr, const size_t size[], const char *const values[], size_t nValues, size_t* index) {
    UNUSED(size);
    SET_NOT_ALLOWED("Binary");
}
#endif

#ifndef CONFIGURATE

#ifdef N_CONTINUOUS_STATES
Status getNumberOfContinuousStates(ModelInstance* instance, size_t* nContinuousStates) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nContinuousStates);
    *nContinuousStates = N_CONTINUOUS_STATES;
    return OK;
}
#else
Status getNumberOfContinuousStates(ModelInstance* instance, size_t* nContinuousStates) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nContinuousStates);
    *nContinuousStates = 0;
    return OK;
}

Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    UNUSED(x);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    UNUSED(nominals);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    UNUSED(x);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    UNUSED(dx);
    ASSERT_SIZE_T(nx, 0);
    return OK;
}
#endif // N_CONTINUOUS_STATES

#ifdef N_EVENT_INDICATORS
Status getNumberOfEventIndicators(ModelInstance* instance, size_t* nEventIndicators) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nEventIndicators);
    *nEventIndicators = N_EVENT_INDICATORS;
    return OK;
}
#else
Status getNumberOfEventIndicators(ModelInstance* instance, size_t* nEventIndicators) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nEventIndicators);
    *nEventIndicators = 0;
    return OK;
}

Status getEventIndicators(ModelInstance *instance, double z[], size_t nz) {
    ASSERT_NOT_NULL2(instance);
    UNUSED(z);
    ASSERT_SIZE_T(nz, 0);
    return OK;
}
#endif // N_EVENT_INDICATORS

#endif // CONFIGURATE

#ifndef GET_PARTIAL_DERIVATIVE
Status getPartialDerivative(ModelInstance *instance, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double* partialDerivative) {
    UNUSED(instance);
    UNUSED(unknown);
    UNUSED(unknownIndex);
    UNUSED(known);
    UNUSED(knownIndex);
    UNUSED(partialDerivative);
    logError(instance, "Directional derivatives are not supported.");
    return Error;
}
#endif

#ifndef ACTIVATE_CLOCK
Status activateClock(ModelInstance *instance, ValueReference vr) {
    UNUSED(instance);
    UNUSED(vr);
    return Error;
}
#endif

#ifndef GET_CLOCK
Status getClock(ModelInstance *instance, ValueReference vr, bool* value) {
    UNUSED(instance);
    UNUSED(vr);
    UNUSED(value);
    return Error;
}
#endif

#ifndef GET_INTERVAL
Status getInterval(ModelInstance *instance, ValueReference vr, double* interval, int* qualifier) {
    UNUSED(instance);
    UNUSED(vr);
    UNUSED(interval);
    UNUSED(qualifier);
    return Error;
}
#endif

#ifndef ACTIVATE_MODEL_PARTITION
Status activateModelPartition(ModelInstance *instance, ValueReference vr, double activationTime) {
    UNUSED(instance);
    UNUSED(vr);
    UNUSED(activationTime);
    return Error;
}
#endif

Status getFMUState(ModelInstance *instance, void** FMUState) {

    CALL(s_reallocate(instance, FMUState, sizeof(ModelInstance)));

    memcpy(*FMUState, instance, sizeof(ModelInstance));

    return OK;
}

Status setFMUState(ModelInstance *instance, void* FMUState) {

    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(FMUState);

    ModelInstance* s = (ModelInstance*)FMUState;

    instance->startTime = s->startTime;
    instance->stopTime = s->stopTime;
    instance->time = s->time;
    instance->status = s->status;
    instance->state = s->state;
    instance->clocksTicked = s->clocksTicked;

    instance->isDirtyValues = s->isDirtyValues;

    memcpy(& instance->modelData, & s->modelData, sizeof(ModelData));

    instance->nSteps = s->nSteps;

    instance->earlyReturnAllowed = s->earlyReturnAllowed;
    instance->eventModeUsed = s->eventModeUsed;
    instance->nextCommunicationPoint = s->nextCommunicationPoint;

#if MAX_EVENT_INDICATORS > 0
    instance->nz = s->nz;
    memcpy(instance->z, s->z, s->nz * sizeof(double));
    memcpy(instance->prez, s->prez, s->nz * sizeof(double));
#endif

#if MAX_CONTINUOUS_STATES > 0
    instance->nx = s->nx;
    memcpy(instance->x, s->x, s->nx * sizeof(double));
    memcpy(instance->dx, s->dx, s->nx * sizeof(double));
#endif

    instance->nSteps = s->nSteps;

    return OK;
}

Status serializeFMUState(
    ModelInstance *instance,
    void*  FMUState,
    uint8_t serializedState[],
    size_t size) {

    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(FMUState);
    ASSERT_NOT_NULL2(serializedState);

    if (invalidNumber(instance, "serializeFMUState", "size", size, sizeof(ModelInstance))) {
        return Error;
    }

    memcpy(serializedState, FMUState, size);
    return OK;
}

Status deserializeFMUState(
    ModelInstance *instance,
    const uint8_t serializedState[],
    size_t size,
    void** FMUState
) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(serializedState);
    ASSERT_NOT_NULL2(FMUState);

    if (invalidNumber(instance, "deserializeFMUState", "size", size, sizeof(ModelInstance))) {
        return Error;
    }

    if (*FMUState == NULL) {
        *FMUState = calloc(1, sizeof(ModelInstance));
        if (*FMUState == NULL) {
            logError(instance, "Failed to allocate memory for FMUState.");
            return Error;
        }
    }

    memcpy(*FMUState, serializedState, size);
    return OK;
}
