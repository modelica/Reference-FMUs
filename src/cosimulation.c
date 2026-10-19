#include <stdlib.h>  // for calloc(), free()
#include <float.h>   // for DBL_EPSILON
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include "config.h"
#include "cosimulation.h"

#if FMI_VERSION == 3
#include "fmi3Functions.h"
#endif

#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifdef CALL
#undef CALL
#endif

#define CALL(f) do { const Status status = f; if (status != OK) return status; } while (false)


ModelInstance *createModelInstance(
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

    comp->newDiscreteStatesNeeded           = false;
    comp->terminateSimulation               = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nextEventTimeDefined              = false;
    comp->nextEventTime                     = 0;

    setStartValues(comp);

    comp->isDirtyValues = true;

    return comp;
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
    comp->nz = getNumberOfEventIndicators(comp);
    CALL(getEventIndicators(comp, comp->prez, comp->nz));
#endif

#if MAX_CONTINUOUS_STATES > 0
    comp->nx = getNumberOfContinuousStates(comp);
#endif

    return OK;
}

Status reset(ModelInstance* comp) {

    comp->state = Instantiated;
    comp->startTime = 0.0;
    comp->time = 0.0;
    comp->nSteps = 0;
    comp->status = OK;
    setStartValues(comp);
    comp->isDirtyValues = true;

    return OK;
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
Status getPartialDerivative(ModelInstance* comp, ValueReference unknown, ValueReference known, double* partialDerivative) {
    UNUSED(comp);
    UNUSED(unknown);
    UNUSED(known);
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

/* FMU state blob (little-endian wire format, no pointers). */

typedef struct {
    uint8_t *data;
    size_t size;
    size_t pos;
} SerializedBuffer;

static void sb_init(SerializedBuffer *sb, uint8_t *data, size_t size) {
    sb->data = data;
    sb->size = size;
    sb->pos = 0;
}

static bool sb_remaining(const SerializedBuffer *sb, size_t n) {
    return sb->pos + n <= sb->size;
}

static bool sb_write_u8(SerializedBuffer *sb, uint8_t value) {
    if (!sb_remaining(sb, 1)) {
        return false;
    }
    sb->data[sb->pos++] = value;
    return true;
}

static bool sb_read_u8(const SerializedBuffer *sb, size_t *pos, uint8_t *value) {
    if (*pos + 1 > sb->size) {
        return false;
    }
    *value = sb->data[(*pos)++];
    return true;
}

static bool sb_write_u32(SerializedBuffer *sb, uint32_t value) {
    if (!sb_remaining(sb, 4)) {
        return false;
    }
    sb->data[sb->pos++] = (uint8_t)(value);
    sb->data[sb->pos++] = (uint8_t)(value >> 8);
    sb->data[sb->pos++] = (uint8_t)(value >> 16);
    sb->data[sb->pos++] = (uint8_t)(value >> 24);
    return true;
}

static bool sb_read_u32(const SerializedBuffer *sb, size_t *pos, uint32_t *value) {
    if (*pos + 4 > sb->size) {
        return false;
    }
    *value = (uint32_t)sb->data[*pos]
        | ((uint32_t)sb->data[*pos + 1] << 8)
        | ((uint32_t)sb->data[*pos + 2] << 16)
        | ((uint32_t)sb->data[*pos + 3] << 24);
    *pos += 4;
    return true;
}

static bool sb_write_u64(SerializedBuffer *sb, uint64_t value) {
    if (!sb_remaining(sb, 8)) {
        return false;
    }
    for (int i = 0; i < 8; i++) {
        sb->data[sb->pos++] = (uint8_t)(value >> (8 * i));
    }
    return true;
}

static bool sb_read_u64(const SerializedBuffer *sb, size_t *pos, uint64_t *value) {
    if (*pos + 8 > sb->size) {
        return false;
    }
    *value = 0;
    for (int i = 0; i < 8; i++) {
        *value |= (uint64_t)sb->data[*pos + i] << (8 * i);
    }
    *pos += 8;
    return true;
}

static bool sb_write_f64(SerializedBuffer *sb, double value) {
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return sb_write_u64(sb, bits);
}

static bool sb_read_f64(const SerializedBuffer *sb, size_t *pos, double *value) {
    uint64_t bits = 0;
    if (!sb_read_u64(sb, pos, &bits)) {
        return false;
    }
    memcpy(value, &bits, sizeof(*value));
    return true;
}

static bool sb_write_bool(SerializedBuffer *sb, bool value) {
    return sb_write_u8(sb, value ? 1u : 0u);
}

static bool sb_read_bool(const SerializedBuffer *sb, size_t *pos, bool *value) {
    uint8_t b = 0;
    if (!sb_read_u8(sb, pos, &b)) {
        return false;
    }
    *value = b != 0;
    return true;
}

static bool sb_write_bytes(SerializedBuffer *sb, const void *src, size_t n) {
    if (!sb_remaining(sb, n)) {
        return false;
    }
    memcpy(sb->data + sb->pos, src, n);
    sb->pos += n;
    return true;
}

static bool sb_read_bytes(const SerializedBuffer *sb, size_t *pos, void *dst, size_t n) {
    if (*pos + n > sb->size) {
        return false;
    }
    memcpy(dst, sb->data + *pos, n);
    *pos += n;
    return true;
}

static size_t serialized_instantiation_token_size(void) {
    return sizeof(uint32_t) + strlen(INSTANTIATION_TOKEN);
}

static bool sb_write_instantiation_token(SerializedBuffer *sb) {
    const char *token = INSTANTIATION_TOKEN;
    const uint32_t len = (uint32_t)strlen(token);
    return sb_write_u32(sb, len) && sb_write_bytes(sb, token, len);
}

static bool sb_read_instantiation_token(const SerializedBuffer *sb, size_t *pos) {
    uint32_t len = 0;
    const char *expected = INSTANTIATION_TOKEN;
    const size_t expected_len = strlen(expected);

    if (!sb_read_u32(sb, pos, &len) || len != expected_len || *pos + len > sb->size) {
        return false;
    }
    if (memcmp(sb->data + *pos, expected, len) != 0) {
        return false;
    }
    *pos += len;
    return true;
}

size_t serializedFMUStateSize(const ModelInstance *comp) {
    size_t size = 0;
#if MAX_EVENT_INDICATORS > 0
    const size_t nz = comp ? comp->nz : 0;
#else
    const size_t nz = 0;
#endif
#if MAX_CONTINUOUS_STATES > 0
    const size_t nx = comp ? comp->nx : 0;
#else
    const size_t nx = 0;
#endif

    size += serialized_instantiation_token_size();
    size += 3 * sizeof(double); /* startTime, stopTime, time */
    size += 2 * sizeof(uint32_t); /* status, state */
    size += 7 * sizeof(uint8_t); /* event / dirty flags */
    size += sizeof(double); /* nextEventTime */
    size += sizeof(uint64_t); /* nSteps */
    size += 2 * sizeof(uint8_t); /* earlyReturnAllowed, eventModeUsed */
    size += sizeof(double); /* nextCommunicationPoint */
    size += sizeof(uint32_t) + nz * 2 * sizeof(double);
    size += sizeof(uint32_t) + nx * 2 * sizeof(double);
    size += sizeof(uint32_t) + sizeof(ModelData);
    return size;
}

Status serializeFMUState(const ModelInstance *comp, uint8_t *buffer, size_t size) {
    SerializedBuffer sb;
    sb_init(&sb, buffer, size);

    if (!comp || !buffer) {
        return Error;
    }

    if (size != serializedFMUStateSize(comp)) {
        return Error;
    }

    if (!sb_write_instantiation_token(&sb)
        || !sb_write_f64(&sb, comp->startTime)
        || !sb_write_f64(&sb, comp->stopTime)
        || !sb_write_f64(&sb, comp->time)
        || !sb_write_u32(&sb, (uint32_t)comp->status)
        || !sb_write_u32(&sb, (uint32_t)comp->state)
        || !sb_write_bool(&sb, comp->newDiscreteStatesNeeded)
        || !sb_write_bool(&sb, comp->terminateSimulation)
        || !sb_write_bool(&sb, comp->nominalsOfContinuousStatesChanged)
        || !sb_write_bool(&sb, comp->valuesOfContinuousStatesChanged)
        || !sb_write_bool(&sb, comp->nextEventTimeDefined)
        || !sb_write_bool(&sb, comp->clocksTicked)
        || !sb_write_bool(&sb, comp->isDirtyValues)
        || !sb_write_f64(&sb, comp->nextEventTime)
        || !sb_write_u64(&sb, comp->nSteps)
        || !sb_write_bool(&sb, comp->earlyReturnAllowed)
        || !sb_write_bool(&sb, comp->eventModeUsed)
        || !sb_write_f64(&sb, comp->nextCommunicationPoint)) {
        return Error;
    }

    {
#if MAX_EVENT_INDICATORS > 0
        if (!sb_write_u32(&sb, (uint32_t)comp->nz)) {
            return Error;
        }
        for (size_t i = 0; i < comp->nz; i++) {
            if (!sb_write_f64(&sb, comp->z[i]) || !sb_write_f64(&sb, comp->prez[i])) {
                return Error;
            }
        }
#else
        if (!sb_write_u32(&sb, 0)) {
            return Error;
        }
#endif
    }

    {
#if MAX_CONTINUOUS_STATES > 0
        if (!sb_write_u32(&sb, (uint32_t)comp->nx)) {
            return Error;
        }
        for (size_t i = 0; i < comp->nx; i++) {
            if (!sb_write_f64(&sb, comp->x[i]) || !sb_write_f64(&sb, comp->dx[i])) {
                return Error;
            }
        }
#else
        if (!sb_write_u32(&sb, 0)) {
            return Error;
        }
#endif
    }

    if (!sb_write_u32(&sb, (uint32_t)sizeof(ModelData))
        || !sb_write_bytes(&sb, &comp->modelData, sizeof(ModelData))) {
        return Error;
    }

    return OK;
}

Status deserializeFMUState(ModelInstance *comp, const uint8_t *buffer, size_t size) {
    SerializedBuffer sb;
    size_t pos = 0;
    uint32_t modelDataSize = 0;
    uint32_t status = 0;
    uint32_t state = 0;

    if (!comp || !buffer) {
        return Error;
    }

    sb_init(&sb, (uint8_t *)buffer, size);

    if (!sb_read_instantiation_token(&sb, &pos)) {
        return Error;
    }

    if (!sb_read_f64(&sb, &pos, &comp->startTime)
        || !sb_read_f64(&sb, &pos, &comp->stopTime)
        || !sb_read_f64(&sb, &pos, &comp->time)
        || !sb_read_u32(&sb, &pos, &status)
        || !sb_read_u32(&sb, &pos, &state)
        || !sb_read_bool(&sb, &pos, &comp->newDiscreteStatesNeeded)
        || !sb_read_bool(&sb, &pos, &comp->terminateSimulation)
        || !sb_read_bool(&sb, &pos, &comp->nominalsOfContinuousStatesChanged)
        || !sb_read_bool(&sb, &pos, &comp->valuesOfContinuousStatesChanged)
        || !sb_read_bool(&sb, &pos, &comp->nextEventTimeDefined)
        || !sb_read_bool(&sb, &pos, &comp->clocksTicked)
        || !sb_read_bool(&sb, &pos, &comp->isDirtyValues)
        || !sb_read_f64(&sb, &pos, &comp->nextEventTime)
        || !sb_read_u64(&sb, &pos, &comp->nSteps)
        || !sb_read_bool(&sb, &pos, &comp->earlyReturnAllowed)
        || !sb_read_bool(&sb, &pos, &comp->eventModeUsed)
        || !sb_read_f64(&sb, &pos, &comp->nextCommunicationPoint)) {
        return Error;
    }

    comp->status = (Status)status;
    comp->state = (ModelState)state;

    {
        uint32_t nz = 0;
        if (!sb_read_u32(&sb, &pos, &nz)) {
            return Error;
        }
#if MAX_EVENT_INDICATORS > 0
        if (nz > MAX_EVENT_INDICATORS) {
            return Error;
        }
        comp->nz = nz;
#else
        if (nz != 0) {
            return Error;
        }
#endif
        for (size_t i = 0; i < nz; i++) {
            double z = 0.0;
            double prez = 0.0;
            if (!sb_read_f64(&sb, &pos, &z) || !sb_read_f64(&sb, &pos, &prez)) {
                return Error;
            }
#if MAX_EVENT_INDICATORS > 0
            comp->z[i] = z;
            comp->prez[i] = prez;
#endif
        }
    }

    {
        uint32_t nx = 0;
        if (!sb_read_u32(&sb, &pos, &nx)) {
            return Error;
        }
#if MAX_CONTINUOUS_STATES > 0
        if (nx > MAX_CONTINUOUS_STATES) {
            return Error;
        }
        comp->nx = nx;
#else
        if (nx != 0) {
            return Error;
        }
#endif
        for (size_t i = 0; i < nx; i++) {
            double x = 0.0;
            double dx = 0.0;
            if (!sb_read_f64(&sb, &pos, &x) || !sb_read_f64(&sb, &pos, &dx)) {
                return Error;
            }
#if MAX_CONTINUOUS_STATES > 0
            comp->x[i] = x;
            comp->dx[i] = dx;
#endif
        }
    }

    if (!sb_read_u32(&sb, &pos, &modelDataSize) || modelDataSize != (uint32_t)sizeof(ModelData)) {
        return Error;
    }
    if (!sb_read_bytes(&sb, &pos, &comp->modelData, sizeof(ModelData))) {
        return Error;
    }

    if (pos != size || size != serializedFMUStateSize(comp)) {
        return Error;
    }

    return OK;
}

Status getFMUState(ModelInstance* comp, void** FMUState) {

    const size_t stateSize = serializedFMUStateSize(comp);

    CALL(s_reallocate(comp, FMUState, stateSize));

    CALL(serializeFMUState(comp, (uint8_t *)*FMUState, stateSize));

    return OK;
}

Status setFMUState(ModelInstance* comp, void* FMUState) {

    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(FMUState);

    return deserializeFMUState(comp, (const uint8_t *)FMUState, serializedFMUStateSize(comp));
}

Status doFixedStep(ModelInstance *comp, bool* stateEvent, bool* timeEvent) {

#if MAX_CONTINUOUS_STATES > 0
    if (comp->nx > 0) {

        CALL(getContinuousStates(comp, comp->x, comp->nx));
        CALL(getDerivatives(comp, comp->dx, comp->nx));

        // forward Euler step
        for (size_t i = 0; i < comp->nx; i++) {
            comp->x[i] += FIXED_SOLVER_STEP * comp->dx[i];
        }

        CALL(setContinuousStates(comp, comp->x, comp->nx));
    }
#endif

    comp->nSteps++;

    comp->time = comp->startTime + comp->nSteps * FIXED_SOLVER_STEP;

    // state event
    *stateEvent = false;

#if MAX_EVENT_INDICATORS > 0
    if (comp->nz > 0) {

        CALL(getEventIndicators(comp, comp->z, comp->nz));

        // check for zero-crossings
        for (size_t i = 0; i < comp->nz; i++) {
            *stateEvent |=
                (comp->prez[i] <= 0 && comp->z[i] >  0) ||
                (comp->prez[i] >  0 && comp->z[i] <= 0);
        }

        // remember the current event indicators
        memcpy(comp->prez, comp->z, comp->nz * sizeof(double));
    }
#endif

    // time event
    *timeEvent = comp->nextEventTimeDefined && comp->time >= comp->nextEventTime;

    bool earlyReturnRequested;
    double earlyReturnTime;

    // intermediate update
    if (comp->intermediateUpdate) {
        comp->intermediateUpdate(
            comp->componentEnvironment, // instanceEnvironment
            comp->time,                 // intermediateUpdateTime
            false,                      // intermediateVariableSetRequested
            true,                       // intermediateVariableGetAllowed
            true,                       // intermediateStepFinished
            false,                      // canReturnEarly
            &earlyReturnRequested,      // earlyReturnRequested
            &earlyReturnTime);          // earlyReturnTime
    }

    return OK;
}
