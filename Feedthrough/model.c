#include "config.h"
#include "model.h"
#include <stdlib.h>  // for free()
#include <string.h>  // for strcmp()


const char *STRING_START = "Set me!";
const char *BINARY_START = "Set me, too!";

void setStartValues(ModelInstance *comp) {
    M(real_fixed_parameter)   = 0;
    M(real_tunable_parameter) = 0;
    M(real_continuous_in)     = 0;
    M(real_discrete)          = 0;
    M(integer)                = 0;
    M(boolean)                = false;
    M(string)                 = STRING_START;
    M(binary_size)            = strlen(BINARY_START);
    M(binary)                 = BINARY_START;
}

void calculateValues(ModelInstance *comp) {
    UNUSED(comp)
    // do nothing
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_continuous_real_in:
            value[(*index)++] = M(real_continuous_in);
            return OK;
        case vr_continuous_real_out:
            value[(*index)++] = M(real_fixed_parameter) + M(real_tunable_parameter) + M(real_continuous_in);
            return OK;
        case vr_discrete_real_in:
        case vr_discrete_real_out:
            value[(*index)++] = M(real_discrete);
            return OK;
        case vr_fixed_real_parameter:
            value[(*index)++] = M(real_fixed_parameter);
            return OK;
        case vr_tunable_real_parameter:
            value[(*index)++] = M(real_tunable_parameter);
            return OK;
        default: return Error;
    }
}

Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_int_in:
            value[(*index)++] = M(integer);
            return OK;
        case vr_int_out:
            value[(*index)++] = M(integer);
            return OK;
        default: return Error;
    }
}

Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_bool_in:
            value[(*index)++] = M(boolean);
            return OK;
        case vr_bool_out:
            value[(*index)++] = M(boolean) && (strcmp(M(string), "FMI is awesome!") == 0);
            return OK;
        default: return Error;
    }
}

Status getBinary(ModelInstance* comp, ValueReference vr, size_t size[], const char *value[], size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_binary_in:
        case vr_binary_out:
            value[*index] = M(binary);
            size[(*index)++] = M(binary_size);
            return OK;
        default: return Error;
    }
    return Error;
}

Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index) {
    switch (vr) {
        case vr_string:
            value[(*index)++] = M(string);
            return OK;
        default: return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {

        case vr_fixed_real_parameter:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode) {
                logError(comp, "Variable fixed_real_parameter can only be set after instantiation or in initialization mode.");
                return Error;
            }
#endif
            M(real_fixed_parameter) = value[(*index)++];
            return OK;

        case vr_tunable_real_parameter:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable tunable_real_parameter can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
#endif
            M(real_tunable_parameter) = value[(*index)++];
            return OK;

        case vr_continuous_real_in:
            M(real_continuous_in) = value[(*index)++];
            return OK;

        case vr_discrete_real_in:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable real_in can only be set in initialization mode or event mode.");
                return Error;
            }
#endif
            M(real_discrete) = value[(*index)++];
            return OK;

        default:
            return Error;
    }
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {
    switch (vr) {
        case vr_int_in:
            M(integer) = value[(*index)++];
            return OK;
        default:
            return Error;
    }
}

Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index) {
    switch (vr) {
        case vr_bool_in:
            M(boolean) = value[(*index)++];
            return OK;
        default:
            return Error;
    }
}

Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index) {
    switch (vr) {
        case vr_string:
            if (M(string) != STRING_START) {
                free((void *)M(string));
            }
            M(string) = strdup(value[(*index)++]);
            return OK;
        default:
            return Error;
    }
}

Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char *const value[], size_t *index) {
    switch (vr) {
        case vr_binary_in:
            if (M(binary) != BINARY_START) {
                free((void *)M(binary));
            }
            M(binary_size) = size[*index];
            M(binary) = calloc(1, M(binary_size));
            memcpy((void *)M(binary), value[(*index)++], M(binary_size));
            return OK;
        default:
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
