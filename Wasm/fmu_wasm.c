/*
 * Reference FMUs - FMI 3.0 Co-Simulation as a WebAssembly component.
 *
 * WIT exports delegate to the selected model via model.h / common.c.
 * Build with -I pointing at the model directory for config.h.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

struct exports_fmi_fmi3_co_simulation_co_simulation_instance_t {
    void *comp;
};

#include "co_simulation_fmu.h"

#define FMI_VERSION 3
#include "config.h"
#include "model.h"
#include "model_instance.h"
#include "cosimulation.h"

static const char WASM_FMI_VERSION[] = "3.0";

static ModelInstance *wasm_comp(exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self) {
    return self ? (ModelInstance *)self->comp : NULL;
}

static exports_fmi_fmi3_co_simulation_status_t map_status(Status s) {
    switch (s) {
        case OK:      return FMI_FMI3_TYPES_STATUS_OK;
        case Warning: return FMI_FMI3_TYPES_STATUS_WARNING;
        case Discard: return FMI_FMI3_TYPES_STATUS_DISCARD;
        case Error:   return FMI_FMI3_TYPES_STATUS_ERROR;
        case Fatal:   return FMI_FMI3_TYPES_STATUS_FATAL;
        default:      return FMI_FMI3_TYPES_STATUS_ERROR;
    }
}

static Status wasm_apply_status(ModelInstance *instance, Status status) {
    if (!instance) {
        return status;
    }
    if (status == Discard) {
        return status;
    }
    if (status == Error) {
        instance->state = Terminated;
        return status;
    }
    if (status == Fatal) {
        instance->state = StartAndEnd;
        return status;
    }
    return status;
}

static Status wasm_get_float64(
    ModelInstance *instance,
    const ValueReference *value_references,
    size_t n,
    double values[])
{
    if (n == 0) {
        return OK;
    }
    if (instance->isDirtyValues) {
        Status status = calculateValues(instance);
        if (status != OK) {
            return wasm_apply_status(instance, status);
        }
        instance->isDirtyValues = false;
    }
    size_t index = 0;
    for (size_t i = 0; i < n; i++) {
        Status status = getFloat64(instance, value_references[i], values, n, &index);
        if (status != OK) {
            return wasm_apply_status(instance, status);
        }
    }
    if (index != n) {
        logError(instance, "Expected nValues = %zu but was %zu.", n, index);
        return wasm_apply_status(instance, Error);
    }
    return OK;
}

static Status wasm_set_float64(
    ModelInstance *instance,
    const ValueReference *value_references,
    size_t n,
    const double values[])
{
    if (n == 0) {
        return OK;
    }
    size_t index = 0;
    for (size_t i = 0; i < n; i++) {
        Status status = setFloat64(instance, value_references[i], values, n, &index);
        if (status != OK) {
            return wasm_apply_status(instance, status);
        }
    }
    if (index != n) {
        logError(instance, "Expected nValues = %zu but was %zu.", n, index);
        return wasm_apply_status(instance, Error);
    }
    instance->isDirtyValues = true;
    return OK;
}

static char *wit_string_to_c(const co_simulation_fmu_string_t *s) {
    if (!s || !s->ptr) {
        return NULL;
    }
    char *buf = (char *)malloc(s->len + 1);
    if (!buf) {
        return NULL;
    }
    memcpy(buf, s->ptr, s->len);
    buf[s->len] = '\0';
    return buf;
}

static void wasm_log_message(
    void *instanceEnvironment,
    int status,
    const char *category,
    const char *message)
{
    exports_fmi_fmi3_co_simulation_co_simulation_instance_t *wrap =
        (exports_fmi_fmi3_co_simulation_co_simulation_instance_t *)instanceEnvironment;
    ModelInstance *instance = wrap ? (ModelInstance *)wrap->comp : NULL;
    co_simulation_fmu_string_t name;
    co_simulation_fmu_string_t cat;
    co_simulation_fmu_string_t msg;

    if (instance && instance->instanceName) {
        co_simulation_fmu_string_dup(&name, instance->instanceName);
    } else {
        co_simulation_fmu_string_dup(&name, "?");
    }
    co_simulation_fmu_string_dup(&cat, category ? category : "");
    co_simulation_fmu_string_dup(&msg, message ? message : "");

    fmi_fmi3_callbacks_log_message(
        &name,
        (fmi_fmi3_callbacks_status_t)status,
        &cat,
        &msg);

    co_simulation_fmu_string_free(&name);
    co_simulation_fmu_string_free(&cat);
    co_simulation_fmu_string_free(&msg);
}

static void wasm_intermediate_update(
    void *instanceEnvironment,
    double intermediateUpdateTime,
    bool intermediateVariableSetRequested,
    bool intermediateVariableGetAllowed,
    bool intermediateStepFinished,
    bool canReturnEarly,
    bool *earlyReturnRequested,
    double *earlyReturnTime)
{
    co_simulation_fmu_tuple2_bool_f64_t ret;
    fmi_fmi3_intermediate_update_callbacks_intermediate_update(
        intermediateUpdateTime,
        intermediateVariableSetRequested,
        intermediateVariableGetAllowed,
        intermediateStepFinished,
        canReturnEarly,
        &ret);
    if (earlyReturnRequested) {
        *earlyReturnRequested = ret.f0;
    }
    if (earlyReturnTime) {
        *earlyReturnTime = ret.f1;
    }
}

void exports_fmi_fmi3_common_get_version(co_simulation_fmu_string_t *ret) {
    const char *version = WASM_FMI_VERSION;
    size_t len = strlen(version);
    uint8_t *buf = (uint8_t *)malloc(len);
    if (buf) {
        memcpy(buf, version, len);
    }
    ret->ptr = buf;
    ret->len = len;
}

bool exports_fmi_fmi3_co_simulation_static_co_simulation_instance_instantiate_co_simulation(
    co_simulation_fmu_string_t *instance_name,
    co_simulation_fmu_string_t *instantiation_token,
    co_simulation_fmu_string_t *resource_path,
    bool visible,
    bool logging_on,
    bool event_mode_used,
    bool early_return_allowed,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *required_intermediate_variables,
    exports_fmi_fmi3_co_simulation_own_co_simulation_instance_t *ret)
{
    exports_fmi_fmi3_co_simulation_co_simulation_instance_t *inst =
        (exports_fmi_fmi3_co_simulation_co_simulation_instance_t *)calloc(1, sizeof(*inst));
    if (!inst) {
        return false;
    }

    char *name = wit_string_to_c(instance_name);
    char *token = wit_string_to_c(instantiation_token);
    char *resources = wit_string_to_c(resource_path);
    if (!name || !token) {
        free(name);
        free(token);
        free(resources);
        free(inst);
        return false;
    }

    (void)visible;
    (void)required_intermediate_variables;

    const char *resource_location = NULL;
    if (resources && resources[0] != '\0') {
        resource_location = resources;
    }

    ModelInstance *instance = createModelInstance(
        wasm_log_message,
        wasm_intermediate_update,
        inst,
        name,
        token,
        resource_location,
        logging_on,
        CoSimulation);

    free(name);
    free(token);
    free(resources);

    if (!instance) {
        free(inst);
        return false;
    }

    instance->earlyReturnAllowed = early_return_allowed;
    instance->eventModeUsed = event_mode_used;
    instance->state = Instantiated;
    inst->comp = instance;

    *ret = exports_fmi_fmi3_co_simulation_co_simulation_instance_new(inst);
    return true;
}

void exports_fmi_fmi3_co_simulation_co_simulation_instance_destructor(
    exports_fmi_fmi3_co_simulation_co_simulation_instance_t *rep)
{
    if (!rep) {
        return;
    }
    if (rep->comp) {
        freeModelInstance((ModelInstance *)rep->comp);
    }
    free(rep);
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_debug_logging(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    bool logging_on,
    co_simulation_fmu_list_string_t *categories)
{
    size_t n = categories ? categories->len : 0;
    const char **cat_ptrs = NULL;
    char **bufs = NULL;

    if (n > 0) {
        cat_ptrs = (const char **)calloc(n, sizeof(char *));
        bufs = (char **)calloc(n, sizeof(char *));
        if (!cat_ptrs || !bufs) {
            free(cat_ptrs);
            free(bufs);
            return FMI_FMI3_TYPES_STATUS_ERROR;
        }
        for (size_t i = 0; i < n; i++) {
            bufs[i] = wit_string_to_c(&categories->ptr[i]);
            cat_ptrs[i] = bufs[i];
        }
    }

    Status st = setDebugLogging(wasm_comp(self), logging_on, n, cat_ptrs);

    if (bufs) {
        for (size_t i = 0; i < n; i++) {
            free(bufs[i]);
        }
        free(bufs);
        free(cat_ptrs);
    }

    return map_status(wasm_apply_status(wasm_comp(self), st));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_initialization_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    double *maybe_tolerance,
    double start_time,
    double *maybe_stop_time)
{
    ModelInstance *instance = wasm_comp(self);
    const bool toleranceDefined = maybe_tolerance != NULL;
    const double tolerance = toleranceDefined ? *maybe_tolerance : 0.0;
    const bool stopTimeDefined = maybe_stop_time != NULL;
    const double stopTime = stopTimeDefined ? *maybe_stop_time : 0.0;

    Status status = setupExperiment(
        instance,
        toleranceDefined,
        tolerance,
        start_time,
        stopTimeDefined,
        stopTime);
    if (status != OK) {
        return map_status(wasm_apply_status(instance, status));
    }

    status = enterInitializationMode(instance);
    return map_status(wasm_apply_status(instance, status));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_exit_initialization_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    Status status = exitInitializationMode(instance);
    return map_status(wasm_apply_status(instance, status));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_event_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    Status status = enterEventMode(instance);
    return map_status(wasm_apply_status(instance, status));
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_update_discrete_states(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_discrete_states_info_t *ok,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    bool discreteStatesNeedUpdate = false;
    bool terminateSimulation = false;
    bool nominalsOfContinuousStatesChanged = false;
    bool valuesOfContinuousStatesChanged = false;
    bool nextEventTimeDefined = false;
    double nextEventTime = 0.0;

#ifdef EVENT_UPDATE
    Status st = updateDiscreteStates(
        instance,
        &discreteStatesNeedUpdate,
        &terminateSimulation,
        &nominalsOfContinuousStatesChanged,
        &valuesOfContinuousStatesChanged,
        &nextEventTimeDefined,
        &nextEventTime);
    if (st != OK && st != Warning) {
        *err = map_status(wasm_apply_status(instance, st));
        return false;
    }
#else
    Status st = OK;
    (void)st;
#endif

    ok->new_discrete_states_needed = discreteStatesNeedUpdate;
    ok->terminate_simulation = terminateSimulation;
    ok->nominals_of_continuous_states_changed = nominalsOfContinuousStatesChanged;
    ok->values_of_continuous_states_changed = valuesOfContinuousStatesChanged;
    ok->next_event_time_defined = nextEventTimeDefined;
    ok->next_event_time = nextEventTime;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_terminate(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    Status status = terminate(instance);
    return map_status(wasm_apply_status(instance, status));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_reset(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    return map_status(wasm_apply_status(instance, reset(instance)));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_configuration_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    Status status = enterConfigurationMode(instance);
    return map_status(wasm_apply_status(instance, status));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_exit_configuration_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    Status status = exitConfigurationMode(instance);
    return map_status(wasm_apply_status(instance, status));
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_float64(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    size_t n = value_references->len;
    double *buf = (double *)calloc(n, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
    ModelInstance *instance = wasm_comp(self);
    Status st = wasm_get_float64(instance, value_references->ptr, n, buf);
    if (st != OK && st != Warning) {
        free(buf);
        *err = map_status(st);
        return false;
    }
    ret->ptr = buf;
    ret->len = n;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_float64(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *values)
{
    if (value_references->len != values->len) {
        return FMI_FMI3_TYPES_STATUS_ERROR;
    }
    ModelInstance *instance = wasm_comp(self);
    return map_status(wasm_set_float64(
        instance,
        value_references->ptr,
        value_references->len,
        values->ptr));
}

#define STUB_GET(T, list_type) \
bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_##T( \
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self, \
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references, \
    co_simulation_fmu_list_##list_type##_t *ret, \
    exports_fmi_fmi3_co_simulation_status_t *err) \
{ \
    (void)self; (void)value_references; (void)ret; \
    *err = FMI_FMI3_TYPES_STATUS_ERROR; \
    return false; \
}

STUB_GET(float32, f32)
STUB_GET(int8, s8)
STUB_GET(int16, s16)
STUB_GET(int32, s32)
STUB_GET(int64, s64)
STUB_GET(uint8, u8)
STUB_GET(uint16, u16)
STUB_GET(uint32, u32)
STUB_GET(uint64, u64)
STUB_GET(boolean, bool)
STUB_GET(string, string)
STUB_GET(binary, list_u8)
STUB_GET(clock, bool)

#define STUB_SET(T, list_type) \
exports_fmi_fmi3_co_simulation_status_t \
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_##T( \
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self, \
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references, \
    co_simulation_fmu_list_##list_type##_t *values) \
{ \
    (void)self; (void)value_references; (void)values; \
    return FMI_FMI3_TYPES_STATUS_ERROR; \
}

STUB_SET(float32, f32)
STUB_SET(int8, s8)
STUB_SET(int16, s16)
STUB_SET(int32, s32)
STUB_SET(int64, s64)
STUB_SET(uint8, u8)
STUB_SET(uint16, u16)
STUB_SET(uint32, u32)
STUB_SET(uint64, u64)
STUB_SET(boolean, bool)
STUB_SET(string, string)
STUB_SET(binary, list_u8)
STUB_SET(clock, bool)

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_number_of_variable_dependencies(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_value_reference_t vr,
    uint64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    (void)vr;
    logError(instance, "Function is not implemented.");
    *err = map_status(wasm_apply_status(instance, Error));
    return false;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_variable_dependencies(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_value_reference_t dependent,
    exports_fmi_fmi3_co_simulation_list_variable_dependency_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    (void)dependent;
    (void)ret;
    logError(instance, "Function is not implemented.");
    *err = map_status(wasm_apply_status(instance, Error));
    return false;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_fmu_state(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_fmu_state_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    void *state = NULL;
    Status st = getFMUState(instance, &state);
    if (st != OK && st != Warning) {
        *err = map_status(wasm_apply_status(instance, st));
        return false;
    }

    ret->ptr = (uint8_t *)state;
    ret->len = sizeof(ModelInstance);
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_fmu_state(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_fmu_state_t *state)
{
    ModelInstance *instance = wasm_comp(self);
    Status st = setFMUState(instance, state->ptr);
    return map_status(wasm_apply_status(instance, st));
}

static Status wasm_get_directional_derivative(
    ModelInstance *instance,
    const ValueReference unknowns[],
    size_t nUnknowns,
    const ValueReference knowns[],
    size_t nKnowns,
    const double seeds[],
    size_t nSeeds,
    double sensitivities[],
    size_t nSensitivities)
{
    size_t sensitivityIndex = 0;

    for (size_t i = 0; i < nUnknowns; i++) {
        const ValueReference unknown = unknowns[i];

        size_t unknownSize = 0;
        Status status = getVariableSize(instance, unknown, &unknownSize);
        if (status != OK) {
            return status;
        }

        for (size_t ii = 0; ii < unknownSize; ii++) {
            double sensitivity = 0.0;
            size_t seedIndex = 0;

            for (size_t j = 0; j < nKnowns; j++) {
                const ValueReference known = knowns[j];

                size_t knownSize = 0;
                status = getVariableSize(instance, known, &knownSize);
                if (status != OK) {
                    return status;
                }

                for (size_t jj = 0; jj < knownSize; jj++) {
                    if (seedIndex >= nSeeds) {
                        logError(instance, "Expected nSeeds > %zu", nSeeds);
                        return Error;
                    }

                    const double seed = seeds[seedIndex++];
                    double partialDerivative = 0.0;

                    status = getPartialDerivative(
                        instance,
                        unknown,
                        ii,
                        known,
                        jj,
                        &partialDerivative);
                    if (status != OK) {
                        return status;
                    }

                    sensitivity += partialDerivative * seed;
                }
            }

            if (sensitivityIndex >= nSensitivities) {
                logError(instance, "Expected nSensitivities > %zu", nSensitivities);
                return Error;
            }

            sensitivities[sensitivityIndex++] = sensitivity;
        }
    }

    return OK;
}

static Status wasm_get_adjoint_derivative(
    ModelInstance *instance,
    const ValueReference unknowns[],
    size_t nUnknowns,
    const ValueReference knowns[],
    size_t nKnowns,
    const double seeds[],
    size_t nSeeds,
    double sensitivities[],
    size_t nSensitivities)
{
    size_t nUnknownElements = 0;
    for (size_t i = 0; i < nUnknowns; i++) {
        size_t unknownSize = 0;
        Status status = getVariableSize(instance, unknowns[i], &unknownSize);
        if (status != OK) {
            return status;
        }
        nUnknownElements += unknownSize;
    }

    size_t nKnownElements = 0;
    for (size_t j = 0; j < nKnowns; j++) {
        size_t knownSize = 0;
        Status status = getVariableSize(instance, knowns[j], &knownSize);
        if (status != OK) {
            return status;
        }
        nKnownElements += knownSize;
    }

    if (nSeeds < nUnknownElements) {
        logError(instance, "Expected nSeeds >= %zu", nUnknownElements);
        return Error;
    }

    if (nSensitivities < nKnownElements) {
        logError(instance, "Expected nSensitivities >= %zu", nKnownElements);
        return Error;
    }

    size_t sensitivityIndex = 0;

    for (size_t j = 0; j < nKnowns; j++) {
        const ValueReference known = knowns[j];

        size_t knownSize = 0;
        Status status = getVariableSize(instance, known, &knownSize);
        if (status != OK) {
            return status;
        }

        for (size_t jj = 0; jj < knownSize; jj++) {
            double s = 0.0;
            size_t seedIndex = 0;

            for (size_t i = 0; i < nUnknowns; i++) {
                const ValueReference unknown = unknowns[i];

                size_t unknownSize = 0;
                status = getVariableSize(instance, unknown, &unknownSize);
                if (status != OK) {
                    return status;
                }

                for (size_t ii = 0; ii < unknownSize; ii++) {
                    if (seedIndex >= nSeeds) {
                        logError(instance, "Expected nSeeds > %zu", nSeeds);
                        return Error;
                    }

                    const double seedValue = seeds[seedIndex++];
                    double partialDerivative = 0.0;

                    status = getPartialDerivative(
                        instance,
                        unknown,
                        ii,
                        known,
                        jj,
                        &partialDerivative);
                    if (status != OK) {
                        return status;
                    }

                    s += partialDerivative * seedValue;
                }
            }

            if (sensitivityIndex >= nSensitivities) {
                logError(instance, "Expected nSensitivities > %zu", nSensitivities);
                return Error;
            }

            sensitivities[sensitivityIndex++] = s;
        }
    }

    return OK;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_directional_derivative(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *unknowns,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *knowns,
    co_simulation_fmu_list_f64_t *seed,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    size_t nUnknownElements = 0;
    for (size_t i = 0; i < unknowns->len; i++) {
        size_t size = 0;
        Status st = getVariableSize(instance, (ValueReference)unknowns->ptr[i], &size);
        if (st != OK) {
            *err = map_status(wasm_apply_status(instance, st));
            return false;
        }
        nUnknownElements += size;
    }

    double *buf = (double *)calloc(nUnknownElements, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }

    Status st = wasm_get_directional_derivative(
        instance,
        unknowns->ptr,
        unknowns->len,
        knowns->ptr,
        knowns->len,
        seed->ptr,
        seed->len,
        buf,
        nUnknownElements);
    if (st != OK && st != Warning) {
        free(buf);
        *err = map_status(wasm_apply_status(instance, st));
        return false;
    }

    ret->ptr = buf;
    ret->len = nUnknownElements;
    return true;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_adjoint_derivative(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *unknowns,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *knowns,
    co_simulation_fmu_list_f64_t *seed,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    size_t nKnownElements = 0;
    for (size_t j = 0; j < knowns->len; j++) {
        size_t size = 0;
        Status st = getVariableSize(instance, (ValueReference)knowns->ptr[j], &size);
        if (st != OK) {
            *err = map_status(wasm_apply_status(instance, st));
            return false;
        }
        nKnownElements += size;
    }

    double *buf = (double *)calloc(nKnownElements, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }

    Status st = wasm_get_adjoint_derivative(
        instance,
        unknowns->ptr,
        unknowns->len,
        knowns->ptr,
        knowns->len,
        seed->ptr,
        seed->len,
        buf,
        nKnownElements);
    if (st != OK && st != Warning) {
        free(buf);
        *err = map_status(wasm_apply_status(instance, st));
        return false;
    }

    ret->ptr = buf;
    ret->len = nKnownElements;
    return true;
}

#define STUB_GET_INTERVAL(name) \
bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_##name( \
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self, \
    exports_fmi_fmi3_co_simulation_list_value_reference_t *vrs, void *ret, \
    exports_fmi_fmi3_co_simulation_status_t *err) \
{ (void)self;(void)vrs;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_interval_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_tuple2_f64_interval_qualifier_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_interval_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_tuple2_interval_fraction_interval_qualifier_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_shift_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_shift_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_interval_fraction_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_interval_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *intervals)
{ (void)self;(void)value_references;(void)intervals; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_interval_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_interval_fraction_t *intervals)
{ (void)self;(void)value_references;(void)intervals; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_shift_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *shifts)
{ (void)self;(void)value_references;(void)shifts; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_shift_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_interval_fraction_t *shifts)
{ (void)self;(void)value_references;(void)shifts; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_evaluate_discrete_states(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    logError(instance, "Function is not implemented.");
    return map_status(wasm_apply_status(instance, Error));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_input_derivatives(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_tuple2_value_reference_u32_t *requests,
    co_simulation_fmu_list_f64_t *values)
{
    (void)self;
    (void)requests;
    (void)values;
    return FMI_FMI3_TYPES_STATUS_ERROR;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_output_derivatives(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_tuple2_value_reference_u32_t *requests,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    size_t n = requests->len;
    ModelInstance *instance = wasm_comp(self);
    double *buf = calloc(n, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
#ifdef GET_OUTPUT_DERIVATIVE
    for (size_t i = 0; i < n; i++) {
        Status st = getOutputDerivative(
            instance,
            (ValueReference)requests->ptr[i].f0,
            (int)requests->ptr[i].f1,
            &buf[i]);
        if (st != OK && st != Warning) {
            free(buf);
            *err = map_status(wasm_apply_status(instance, st));
            return false;
        }
    }
#else
    logError(instance, "Function is not implemented.");
    free(buf);
    *err = map_status(wasm_apply_status(instance, Error));
    return false;
#endif
    ret->ptr = buf;
    ret->len = n;
    return true;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_do_step(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    double current_communication_point,
    double communication_step_size,
    bool no_set_fmu_state_prior_to_current_point,
    exports_fmi_fmi3_co_simulation_do_step_result_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *instance = wasm_comp(self);
    bool eventHandlingNeeded = false;
    bool terminateSimulation = false;
    bool earlyReturn = false;
    double lastSuccessfulTime = 0.0;

    Status st = doStep(
        instance,
        current_communication_point,
        communication_step_size,
        no_set_fmu_state_prior_to_current_point,
        &eventHandlingNeeded,
        &terminateSimulation,
        &earlyReturn,
        &lastSuccessfulTime);

    if (st == Discard) {
        *err = FMI_FMI3_TYPES_STATUS_DISCARD;
        ret->last_successful_time = lastSuccessfulTime;
        ret->event_handling_needed = eventHandlingNeeded;
        ret->terminate_simulation = terminateSimulation;
        ret->early_return = earlyReturn;
        return false;
    }
    if (st != OK && st != Warning) {
        *err = map_status(st);
        return false;
    }

    ret->last_successful_time = lastSuccessfulTime;
    ret->event_handling_needed = eventHandlingNeeded;
    ret->terminate_simulation = terminateSimulation;
    ret->early_return = earlyReturn;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_step_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *instance = wasm_comp(self);
    Status status = enterStepMode(instance);
    return map_status(wasm_apply_status(instance, status));
}
