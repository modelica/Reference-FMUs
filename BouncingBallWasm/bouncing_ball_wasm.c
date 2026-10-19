/*
 * BouncingBallWasm — FMI 3.0 Co-Simulation FMU as a WebAssembly component.
 *
 * WIT exports (co-simulation-fmu world) delegate to the Reference-FMUs
 * BouncingBall model via the standard fmi3* C API (fmi3Functions.c).
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

struct exports_fmi_fmi3_co_simulation_co_simulation_instance_t {
    void *fmi_instance;
};

#include "co_simulation_fmu.h"

#define FMI_VERSION 3
#include "../BouncingBall/config.h"
#include "../include/model.h"
#include "../include/fmi3Functions.h"

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

static exports_fmi_fmi3_co_simulation_status_t map_status(fmi3Status s) {
    switch (s) {
        case fmi3OK:       return FMI_FMI3_TYPES_STATUS_OK;
        case fmi3Warning:  return FMI_FMI3_TYPES_STATUS_WARNING;
        case fmi3Discard:  return FMI_FMI3_TYPES_STATUS_DISCARD;
        case fmi3Error:    return FMI_FMI3_TYPES_STATUS_ERROR;
        case fmi3Fatal:    return FMI_FMI3_TYPES_STATUS_FATAL;
        default:           return FMI_FMI3_TYPES_STATUS_ERROR;
    }
}

static void wasm_log_message(
    void *instanceEnvironment,
    fmi3Status status,
    const char *category,
    const char *message)
{
    exports_fmi_fmi3_co_simulation_co_simulation_instance_t *wrap =
        (exports_fmi_fmi3_co_simulation_co_simulation_instance_t *)instanceEnvironment;
    ModelInstance *comp = wrap ? (ModelInstance *)wrap->fmi_instance : NULL;
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
    const char *version = fmi3GetVersion();
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
    co_simulation_fmu_list_value_reference_t *required_intermediate_variables,
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

    inst->fmi_instance = fmi3InstantiateCoSimulation(
        name,
        token,
        resources ? resources : "",
        visible ? fmi3True : fmi3False,
        logging_on ? fmi3True : fmi3False,
        event_mode_used ? fmi3True : fmi3False,
        early_return_allowed ? fmi3True : fmi3False,
        required_intermediate_variables ? required_intermediate_variables->ptr : NULL,
        required_intermediate_variables ? required_intermediate_variables->len : 0,
        inst,
        wasm_log_message,
        wasm_intermediate_update);

    free(name);
    free(token);
    free(resources);

    if (!inst->fmi_instance) {
        free(inst);
        return false;
    }

    *ret = exports_fmi_fmi3_co_simulation_co_simulation_instance_new(inst);
    return true;
}

void exports_fmi_fmi3_co_simulation_co_simulation_instance_destructor(
    exports_fmi_fmi3_co_simulation_co_simulation_instance_t *rep)
{
    if (!rep) {
        return;
    }
    if (rep->fmi_instance) {
        fmi3FreeInstance((fmi3Instance)rep->fmi_instance);
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

    fmi3Status st = fmi3SetDebugLogging(
        (fmi3Instance)self->fmi_instance,
        logging_on ? fmi3True : fmi3False,
        n,
        cat_ptrs);

    if (bufs) {
        for (size_t i = 0; i < n; i++) {
            free(bufs[i]);
        }
        free(bufs);
        free(cat_ptrs);
    }

    return map_status(st);
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_initialization_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    double *maybe_tolerance,
    double start_time,
    double *maybe_stop_time)
{
    return map_status(fmi3EnterInitializationMode(
        (fmi3Instance)self->fmi_instance,
        maybe_tolerance ? fmi3True : fmi3False,
        maybe_tolerance ? *maybe_tolerance : 0.0,
        start_time,
        maybe_stop_time ? fmi3True : fmi3False,
        maybe_stop_time ? *maybe_stop_time : 0.0));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_exit_initialization_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3ExitInitializationMode((fmi3Instance)self->fmi_instance));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_event_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3EnterEventMode((fmi3Instance)self->fmi_instance));
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_update_discrete_states(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_discrete_states_info_t *ok,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    fmi3Boolean discreteStatesNeedUpdate = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean nominalsChanged = fmi3False;
    fmi3Boolean valuesChanged = fmi3False;
    fmi3Boolean nextEventTimeDefined = fmi3False;
    fmi3Float64 nextEventTime = 0.0;

    fmi3Status st = fmi3UpdateDiscreteStates(
        (fmi3Instance)self->fmi_instance,
        &discreteStatesNeedUpdate,
        &terminateSimulation,
        &nominalsChanged,
        &valuesChanged,
        &nextEventTimeDefined,
        &nextEventTime);

    if (st != fmi3OK && st != fmi3Warning) {
        *err = map_status(st);
        return false;
    }

    ok->new_discrete_states_needed = discreteStatesNeedUpdate != fmi3False;
    ok->terminate_simulation = terminateSimulation != fmi3False;
    ok->nominals_of_continuous_states_changed = nominalsChanged != fmi3False;
    ok->values_of_continuous_states_changed = valuesChanged != fmi3False;
    ok->next_event_time_defined = nextEventTimeDefined != fmi3False;
    ok->next_event_time = nextEventTime;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_terminate(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3Terminate((fmi3Instance)self->fmi_instance));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_reset(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3Reset((fmi3Instance)self->fmi_instance));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_configuration_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3EnterConfigurationMode((fmi3Instance)self->fmi_instance));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_exit_configuration_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3ExitConfigurationMode((fmi3Instance)self->fmi_instance));
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_float64(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    size_t n = value_references->len;
    double *buf = (double *)calloc(n, sizeof(double));
    if (!buf) {
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
    fmi3Status st = fmi3GetFloat64(
        (fmi3Instance)self->fmi_instance,
        value_references->ptr,
        n,
        buf,
        n);
    if (st != fmi3OK && st != fmi3Warning) {
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
    co_simulation_fmu_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *values)
{
    if (value_references->len != values->len) {
        return FMI_FMI3_TYPES_STATUS_ERROR;
    }
    return map_status(fmi3SetFloat64(
        (fmi3Instance)self->fmi_instance,
        value_references->ptr,
        value_references->len,
        values->ptr,
        values->len));
}

#define STUB_GET(T, list_type) \
bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_##T( \
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self, \
    co_simulation_fmu_list_value_reference_t *value_references, \
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
    co_simulation_fmu_list_value_reference_t *value_references, \
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
    size_t n = 0;
    fmi3Status st = fmi3GetNumberOfVariableDependencies((fmi3Instance)self->fmi_instance, vr, &n);
    if (st != fmi3OK && st != fmi3Warning) {
        *err = map_status(st);
        return false;
    }
    *ret = (uint64_t)n;
    return true;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_variable_dependencies(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_value_reference_t dependent,
    exports_fmi_fmi3_co_simulation_list_variable_dependency_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    size_t n = 0;
    fmi3Status st = fmi3GetNumberOfVariableDependencies((fmi3Instance)self->fmi_instance, dependent, &n);
    if (st != fmi3OK && st != fmi3Warning) {
        *err = map_status(st);
        return false;
    }
    if (n == 0) {
        ret->ptr = NULL;
        ret->len = 0;
        return true;
    }

    size_t *elementIndicesOfDependent = calloc(n, sizeof(size_t));
    fmi3ValueReference *independents = calloc(n, sizeof(fmi3ValueReference));
    size_t *elementIndicesOfIndependents = calloc(n, sizeof(size_t));
    fmi3DependencyKind *kinds = calloc(n, sizeof(fmi3DependencyKind));
    exports_fmi_fmi3_co_simulation_variable_dependency_t *deps =
        calloc(n, sizeof(exports_fmi_fmi3_co_simulation_variable_dependency_t));

    if (!elementIndicesOfDependent || !independents || !elementIndicesOfIndependents || !kinds || !deps) {
        free(elementIndicesOfDependent);
        free(independents);
        free(elementIndicesOfIndependents);
        free(kinds);
        free(deps);
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }

    st = fmi3GetVariableDependencies(
        (fmi3Instance)self->fmi_instance,
        dependent,
        elementIndicesOfDependent,
        independents,
        elementIndicesOfIndependents,
        kinds,
        n);

    if (st != fmi3OK && st != fmi3Warning) {
        free(elementIndicesOfDependent);
        free(independents);
        free(elementIndicesOfIndependents);
        free(kinds);
        free(deps);
        *err = map_status(st);
        return false;
    }

    for (size_t i = 0; i < n; i++) {
        deps[i].dependent_value_reference = dependent;
        deps[i].element_index_of_dependent = (uint32_t)elementIndicesOfDependent[i];
        deps[i].independent_value_reference = independents[i];
        deps[i].element_index_of_independent = (uint32_t)elementIndicesOfIndependents[i];
        deps[i].kind = (fmi_fmi3_types_dependency_kind_t)kinds[i];
    }

    free(elementIndicesOfDependent);
    free(independents);
    free(elementIndicesOfIndependents);
    free(kinds);

    ret->ptr = deps;
    ret->len = n;
    return true;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_fmu_state(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_fmu_state_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    fmi3FMUState state = NULL;
    fmi3Status st = fmi3GetFMUState((fmi3Instance)self->fmi_instance, &state);
    if (st != fmi3OK && st != fmi3Warning) {
        *err = map_status(st);
        return false;
    }

    size_t size = 0;
    st = fmi3SerializedFMUStateSize((fmi3Instance)self->fmi_instance, state, &size);
    if (st != fmi3OK && st != fmi3Warning) {
        fmi3FreeFMUState((fmi3Instance)self->fmi_instance, &state);
        *err = map_status(st);
        return false;
    }

    uint8_t *buf = (uint8_t *)malloc(size);
    if (!buf) {
        fmi3FreeFMUState((fmi3Instance)self->fmi_instance, &state);
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }

    st = fmi3SerializeFMUState((fmi3Instance)self->fmi_instance, state, buf, size);
    fmi3FreeFMUState((fmi3Instance)self->fmi_instance, &state);
    if (st != fmi3OK && st != fmi3Warning) {
        free(buf);
        *err = map_status(st);
        return false;
    }

    ret->ptr = buf;
    ret->len = size;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_fmu_state(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    exports_fmi_fmi3_co_simulation_fmu_state_t *state)
{
    fmi3FMUState fmuState = NULL;
    fmi3Status st = fmi3DeserializeFMUState((fmi3Instance)self->fmi_instance, state->ptr, state->len, &fmuState);
    if (st != fmi3OK && st != fmi3Warning) {
        return map_status(st);
    }
    st = fmi3SetFMUState((fmi3Instance)self->fmi_instance, fmuState);
    fmi3FreeFMUState((fmi3Instance)self->fmi_instance, &fmuState);
    return map_status(st);
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_directional_derivative(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *unknowns,
    co_simulation_fmu_list_value_reference_t *knowns,
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
    fmi3Status st = fmi3GetDirectionalDerivative(
        (fmi3Instance)self->fmi_instance,
        unknowns->ptr,
        unknowns->len,
        knowns->ptr,
        knowns->len,
        seed->ptr,
        seed->len,
        buf,
        n);
    if (st != fmi3OK && st != fmi3Warning) {
        free(buf);
        *err = map_status(st);
        return false;
    }
    ret->ptr = buf;
    ret->len = n;
    return true;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_adjoint_derivative(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *unknowns,
    co_simulation_fmu_list_value_reference_t *knowns,
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
    fmi3Status st = fmi3GetAdjointDerivative(
        (fmi3Instance)self->fmi_instance,
        unknowns->ptr,
        unknowns->len,
        knowns->ptr,
        knowns->len,
        seed->ptr,
        seed->len,
        buf,
        n);
    if (st != fmi3OK && st != fmi3Warning) {
        free(buf);
        *err = map_status(st);
        return false;
    }
    ret->ptr = buf;
    ret->len = n;
    return true;
}

#define STUB_GET_INTERVAL(name) \
bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_##name( \
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self, \
    co_simulation_fmu_list_value_reference_t *vrs, void *ret, \
    exports_fmi_fmi3_co_simulation_status_t *err) \
{ (void)self;(void)vrs;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_interval_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_tuple2_f64_interval_qualifier_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_interval_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_tuple2_interval_fraction_interval_qualifier_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_shift_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_shift_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_interval_fraction_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{ (void)self;(void)value_references;(void)ret; *err=FMI_FMI3_TYPES_STATUS_ERROR; return false; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_interval_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *intervals)
{ (void)self;(void)value_references;(void)intervals; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_interval_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_interval_fraction_t *intervals)
{ (void)self;(void)value_references;(void)intervals; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_shift_decimal(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    co_simulation_fmu_list_f64_t *shifts)
{ (void)self;(void)value_references;(void)shifts; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_shift_fraction(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_value_reference_t *value_references,
    exports_fmi_fmi3_co_simulation_list_interval_fraction_t *shifts)
{ (void)self;(void)value_references;(void)shifts; return FMI_FMI3_TYPES_STATUS_ERROR; }

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_evaluate_discrete_states(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3EvaluateDiscreteStates((fmi3Instance)self->fmi_instance));
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_set_input_derivatives(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_tuple2_value_reference_u32_t *requests,
    co_simulation_fmu_list_f64_t *values)
{
    (void)self;
    (void)requests;
    (void)values;
    return FMI_FMI3_TYPES_STATUS_ERROR;
}

bool exports_fmi_fmi3_co_simulation_method_co_simulation_instance_get_output_derivatives(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self,
    co_simulation_fmu_list_tuple2_value_reference_u32_t *requests,
    co_simulation_fmu_list_f64_t *ret,
    exports_fmi_fmi3_co_simulation_status_t *err)
{
    size_t n = requests->len;
    fmi3ValueReference *vrs = calloc(n, sizeof(fmi3ValueReference));
    fmi3Int32 *orders = calloc(n, sizeof(fmi3Int32));
    double *buf = calloc(n, sizeof(double));
    if (!vrs || !orders || !buf) {
        free(vrs);
        free(orders);
        free(buf);
        *err = FMI_FMI3_TYPES_STATUS_ERROR;
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        vrs[i] = requests->ptr[i].f0;
        orders[i] = (fmi3Int32)requests->ptr[i].f1;
    }
    fmi3Status st = fmi3GetOutputDerivatives(
        (fmi3Instance)self->fmi_instance,
        vrs,
        n,
        orders,
        buf,
        n);
    free(vrs);
    free(orders);
    if (st != fmi3OK && st != fmi3Warning) {
        free(buf);
        *err = map_status(st);
        return false;
    }
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
    fmi3Boolean eventHandlingNeeded = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean earlyReturn = fmi3False;
    fmi3Float64 lastSuccessfulTime = 0.0;

    fmi3Status st = fmi3DoStep(
        (fmi3Instance)self->fmi_instance,
        current_communication_point,
        communication_step_size,
        no_set_fmu_state_prior_to_current_point ? fmi3True : fmi3False,
        &eventHandlingNeeded,
        &terminateSimulation,
        &earlyReturn,
        &lastSuccessfulTime);

    if (st == fmi3Discard) {
        *err = FMI_FMI3_TYPES_STATUS_DISCARD;
        ret->last_successful_time = lastSuccessfulTime;
        ret->event_handling_needed = eventHandlingNeeded != fmi3False;
        ret->terminate_simulation = terminateSimulation != fmi3False;
        ret->early_return = earlyReturn != fmi3False;
        return false;
    }
    if (st != fmi3OK && st != fmi3Warning) {
        *err = map_status(st);
        return false;
    }

    ret->last_successful_time = lastSuccessfulTime;
    ret->event_handling_needed = eventHandlingNeeded != fmi3False;
    ret->terminate_simulation = terminateSimulation != fmi3False;
    ret->early_return = earlyReturn != fmi3False;
    return true;
}

exports_fmi_fmi3_co_simulation_status_t
exports_fmi_fmi3_co_simulation_method_co_simulation_instance_enter_step_mode(
    exports_fmi_fmi3_co_simulation_borrow_co_simulation_instance_t self)
{
    return map_status(fmi3EnterStepMode((fmi3Instance)self->fmi_instance));
}
