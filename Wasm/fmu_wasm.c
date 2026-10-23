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

static Status wasm_apply_status(ModelInstance *comp, Status status) {
    if (!comp) {
        return status;
    }
    if (status == Discard) {
        return status;
    }
    if (status == Error) {
        comp->state = Terminated;
        return status;
    }
    if (status == Fatal) {
        comp->state = StartAndEnd;
        return status;
    }
    return status;
}

static Status wasm_get_float64(
    ModelInstance *comp,
    const ValueReference *value_references,
    size_t n,
    double values[])
{
    if (n == 0) {
        return OK;
    }
    if (comp->isDirtyValues) {
        Status status = calculateValues(comp);
        if (status != OK) {
            return wasm_apply_status(comp, status);
        }
        comp->isDirtyValues = false;
    }
    size_t index = 0;
    for (size_t i = 0; i < n; i++) {
        Status status = getFloat64(comp, value_references[i], values, n, &index);
        if (status != OK) {
            return wasm_apply_status(comp, status);
        }
    }
    if (index != n) {
        logError(comp, "Expected nValues = %zu but was %zu.", n, index);
        return wasm_apply_status(comp, Error);
    }
    return OK;
}

static Status wasm_set_float64(
    ModelInstance *comp,
    const ValueReference *value_references,
    size_t n,
    const double values[])
{
    if (n == 0) {
        return OK;
    }
    size_t index = 0;
    for (size_t i = 0; i < n; i++) {
        Status status = setFloat64(comp, value_references[i], values, n, &index);
        if (status != OK) {
            return wasm_apply_status(comp, status);
        }
    }
    if (index != n) {
        logError(comp, "Expected nValues = %zu but was %zu.", n, index);
        return wasm_apply_status(comp, Error);
    }
    comp->isDirtyValues = true;
    return OK;
}

static Status wasm_do_step(
    ModelInstance *S,
    double currentCommunicationPoint,
    double communicationStepSize,
    bool *eventHandlingNeeded,
    bool *terminateSimulation,
    bool *earlyReturn,
    double *lastSuccessfulTime)
{
    if (!isClose(currentCommunicationPoint, S->nextCommunicationPoint)) {
        logError(S, "Expected currentCommunicationPoint = %.16g but was %.16g.",
            S->nextCommunicationPoint, currentCommunicationPoint);
        return wasm_apply_status(S, Error);
    }
    if (communicationStepSize <= 0) {
        logError(S, "Communication step size must be > 0 but was %.16g.", communicationStepSize);
        return wasm_apply_status(S, Error);
    }

    const double nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;
    if (nextCommunicationPoint > S->stopTime && !isClose(nextCommunicationPoint, S->stopTime)) {
        logError(S, "At communication point %.16g a step size of %.16g was requested but stop time is %.16g.",
            currentCommunicationPoint, communicationStepSize, S->stopTime);
        return wasm_apply_status(S, Error);
    }

    bool nextCommunicationPointReached = false;
    *eventHandlingNeeded = false;
    *terminateSimulation = false;
    *earlyReturn = false;

    while (true) {
        const double nextSolverStepTime = S->time + FIXED_SOLVER_STEP;
        nextCommunicationPointReached =
            nextSolverStepTime > nextCommunicationPoint
            && !isClose(nextSolverStepTime, nextCommunicationPoint);

        if (nextCommunicationPointReached || (*eventHandlingNeeded && S->earlyReturnAllowed)) {
            break;
        }

#ifdef EVENT_UPDATE
        if (*eventHandlingNeeded) {
            Status status = eventUpdate(S);
            if (status != OK) {
                return wasm_apply_status(S, status);
            }
            *eventHandlingNeeded = false;
        }
#endif

        bool stateEvent = false;
        bool timeEvent = false;
        Status status = doFixedStep(S, &stateEvent, &timeEvent);
        if (status != OK) {
            return wasm_apply_status(S, status);
        }

#ifdef EVENT_UPDATE
        if (stateEvent || timeEvent) {
            if (S->eventModeUsed) {
                *eventHandlingNeeded = true;
            } else {
                status = eventUpdate(S);
                if (status != OK) {
                    return wasm_apply_status(S, status);
                }
#if MAX_EVENT_INDICATORS > 0
                status = getEventIndicators(S, S->prez, S->nz);
                if (status != OK) {
                    return wasm_apply_status(S, status);
                }
#endif
            }
            if (S->earlyReturnAllowed) {
                break;
            }
        }
#endif

        if (S->terminateSimulation) {
            break;
        }
    }

    *terminateSimulation = S->terminateSimulation;
    *earlyReturn = S->earlyReturnAllowed && !nextCommunicationPointReached;
    *lastSuccessfulTime = S->time;

    if (nextCommunicationPointReached) {
        S->nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;
    } else {
        S->nextCommunicationPoint = S->time;
    }

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
    ModelInstance *comp = wrap ? (ModelInstance *)wrap->comp : NULL;
    co_simulation_fmu_string_t name;
    co_simulation_fmu_string_t cat;
    co_simulation_fmu_string_t msg;

    if (comp && comp->instanceName) {
        co_simulation_fmu_string_dup(&name, comp->instanceName);
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

    ModelInstance *comp = createModelInstance(
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

    if (!comp) {
        free(inst);
        return false;
    }

    comp->earlyReturnAllowed = early_return_allowed;
    comp->eventModeUsed = event_mode_used;
    comp->state = Instantiated;
    inst->comp = comp;

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
    ModelInstance *comp = wasm_comp(self);
    (void)maybe_tolerance;
    comp->startTime = start_time;
    comp->stopTime = maybe_stop_time ? *maybe_stop_time : INFINITY;
    comp->time = start_time;
    comp->nextCommunicationPoint = start_time;
    comp->state = InitializationMode;
    return map_status(OK);
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_exit_initialization_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *comp = wasm_comp(self);
    Status status = OK;
    if (comp->isDirtyValues) {
        status = calculateValues(comp);
        comp->isDirtyValues = false;
    }
    if (status == OK) {
        comp->state = comp->eventModeUsed ? EventMode : StepMode;
        status = configurate(comp);
    }
    return map_status(wasm_apply_status(comp, status));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_event_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *comp = wasm_comp(self);
    comp->state = EventMode;
    return map_status(OK);
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_update_discrete_states(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_discrete_states_info_t *ok,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *comp = wasm_comp(self);
#ifdef EVENT_UPDATE
    Status st = eventUpdate(comp);
    if (st != OK && st != Warning) {
        *err = map_status(wasm_apply_status(comp, st));
        return false;
    }
#else
    Status st = OK;
#endif

    ok->new_discrete_states_needed = comp->newDiscreteStatesNeeded;
    ok->terminate_simulation = comp->terminateSimulation;
    ok->nominals_of_continuous_states_changed = comp->nominalsOfContinuousStatesChanged;
    ok->values_of_continuous_states_changed = comp->valuesOfContinuousStatesChanged;
    ok->next_event_time_defined = comp->nextEventTimeDefined;
    ok->next_event_time = comp->nextEventTime;
    (void)st;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_terminate(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    wasm_comp(self)->state = Terminated;
    return map_status(OK);
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_reset(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *comp = wasm_comp(self);
    return map_status(wasm_apply_status(comp, reset(comp)));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_configuration_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *comp = wasm_comp(self);
    comp->state = (comp->state == Instantiated) ? ConfigurationMode : ReconfigurationMode;
    return map_status(OK);
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_exit_configuration_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    ModelInstance *comp = wasm_comp(self);
    if (comp->state == ConfigurationMode) {
        comp->state = Instantiated;
    } else {
        comp->state = StepMode;
    }
    return map_status(OK);
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
    ModelInstance *comp = wasm_comp(self);
    Status st = wasm_get_float64(comp, value_references->ptr, n, buf);
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
    ModelInstance *comp = wasm_comp(self);
    return map_status(wasm_set_float64(
        comp,
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
    ModelInstance *comp = wasm_comp(self);
    (void)vr;
    logError(comp, "Function is not implemented.");
    *err = map_status(wasm_apply_status(comp, Error));
    return false;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_variable_dependencies(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_value_reference_t dependent,
    exports_fmi_fmi3_co_simulation_list_variable_dependency_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *comp = wasm_comp(self);
    (void)dependent;
    (void)ret;
    logError(comp, "Function is not implemented.");
    *err = map_status(wasm_apply_status(comp, Error));
    return false;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_fmu_state(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_fmu_state_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    ModelInstance *comp = wasm_comp(self);
    void *state = NULL;
    Status st = getFMUState(comp, &state);
    if (st != OK && st != Warning) {
        *err = map_status(wasm_apply_status(comp, st));
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
    ModelInstance *comp = wasm_comp(self);
    Status st = setFMUState(comp, state->ptr);
    return map_status(wasm_apply_status(comp, st));
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_directional_derivative(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *unknowns,
    exports_fmi_fmi3_co_simulation_list_value_reference_t *knowns,
    co_simulation_fmu_list_f64_t *seed,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    size_t n = unknowns->len;
    double *buf = (double *)calloc(n, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
    ModelInstance *comp = wasm_comp(self);
    for (size_t i = 0; i < n; i++) {
        buf[i] = 0.0;
        for (size_t j = 0; j < knowns->len; j++) {
            double partialDerivative = 0.0;
            Status st = getPartialDerivative(
                comp,
                (ValueReference)unknowns->ptr[i],
                (ValueReference)knowns->ptr[j],
                &partialDerivative);
            if (st != OK) {
                free(buf);
                *err = map_status(wasm_apply_status(comp, st));
                return false;
            }
            buf[i] += partialDerivative * seed->ptr[j];
        }
    }
    ret->ptr = buf;
    ret->len = n;
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
    size_t n = knowns->len;
    double *buf = (double *)calloc(n, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
    ModelInstance *comp = wasm_comp(self);
    for (size_t i = 0; i < n; i++) {
        buf[i] = 0.0;
        for (size_t j = 0; j < unknowns->len; j++) {
            double partialDerivative = 0.0;
            Status st = getPartialDerivative(
                comp,
                (ValueReference)unknowns->ptr[j],
                (ValueReference)knowns->ptr[i],
                &partialDerivative);
            if (st != OK) {
                free(buf);
                *err = map_status(wasm_apply_status(comp, st));
                return false;
            }
            buf[i] += partialDerivative * seed->ptr[j];
        }
    }
    ret->ptr = buf;
    ret->len = n;
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
    ModelInstance *comp = wasm_comp(self);
    logError(comp, "Function is not implemented.");
    return map_status(wasm_apply_status(comp, Error));
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
    ModelInstance *comp = wasm_comp(self);
    double *buf = calloc(n, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
#ifdef GET_OUTPUT_DERIVATIVE
    for (size_t i = 0; i < n; i++) {
        Status st = getOutputDerivative(
            comp,
            (ValueReference)requests->ptr[i].f0,
            (int)requests->ptr[i].f1,
            &buf[i]);
        if (st != OK && st != Warning) {
            free(buf);
            *err = map_status(wasm_apply_status(comp, st));
            return false;
        }
    }
#else
    logError(comp, "Function is not implemented.");
    free(buf);
    *err = map_status(wasm_apply_status(comp, Error));
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
    (void)no_set_fmu_state_prior_to_current_point;

    ModelInstance *comp = wasm_comp(self);
    bool eventHandlingNeeded = false;
    bool terminateSimulation = false;
    bool earlyReturn = false;
    double lastSuccessfulTime = 0.0;

    Status st = wasm_do_step(
        comp,
        current_communication_point,
        communication_step_size,
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
    wasm_comp(self)->state = StepMode;
    return map_status(OK);
}
