#include "config.h"
#include "model.h"
#include <stdlib.h>  // for free()
#include <string.h>  // for strcmp()

#ifdef _MSC_VER
#define strdup _strdup
#endif

#define STRING_START "Set me!"
#define BINARY_START "Set me, too!"

void setStartValues(ModelInstance *comp) {

    M(Float32_continuous_input)  = 0.0f;
    M(Float32_continuous_output) = 0.0f;
    M(Float32_discrete_input)    = 0.0f;
    M(Float32_discrete_output)   = 0.0f;
    
    M(Float64_fixed_parameter)   = 0.0;
    M(Float64_tunable_parameter) = 0.0;
    M(Float64_continuous_input)  = 0.0;
    M(Float64_continuous_output) = 0.0;
    M(Float64_discrete_input)    = 0.0;
    M(Float64_discrete_output)   = 0.0;
    
    M(Int32_input)               = 0;
    M(Int32_output)              = 0.0;

    M(Boolean_input)             = false;
    M(Boolean_output)            = false;
    
    strncpy(M(String_parameter), STRING_START, STRING_MAX_LEN);
    
    M(Binary_input_size) = strlen(BINARY_START);
    strncpy(M(Binary_input), BINARY_START, BINARY_MAX_LEN);
    M(Binary_output_size) = strlen(BINARY_START);
    strncpy(M(Binary_output), BINARY_START, BINARY_MAX_LEN);
}

Status calculateValues(ModelInstance *comp) {
    UNUSED(comp);
    // nothing to do
    return OK;
}

Status getFloat32(ModelInstance* comp, ValueReference vr, float *value, size_t *index) {

    calculateValues(comp);

    switch (vr) {
        case vr_Float32_continuous_input:
            value[(*index)++] = M(Float32_continuous_input);
            return OK;
        case vr_Float32_continuous_output:
            value[(*index)++] = M(Float32_continuous_output);
            return OK;
        case vr_Float32_discrete_input:
            value[(*index)++] = M(Float32_discrete_input);
            return OK;
        case vr_Float32_discrete_output:
            value[(*index)++] = M(Float32_discrete_output);
            return OK;
        default:
            logError(comp, "Get Float32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {

    calculateValues(comp);

    switch (vr) {
        case vr_time:
            value[(*index)++] = comp->time;
            return OK;
        case vr_Float64_fixed_parameter:
            value[(*index)++] = M(Float64_fixed_parameter);
            return OK;
        case vr_Float64_tunable_parameter:
            value[(*index)++] = M(Float64_tunable_parameter);
            return OK;
        case vr_Float64_continuous_input:
            value[(*index)++] = M(Float64_continuous_input);
            return OK;
        case vr_Float64_continuous_output:
            value[(*index)++] = M(Float64_continuous_output);
            return OK;
        case vr_Float64_discrete_input:
            value[(*index)++] = M(Float64_discrete_input);
            return OK;
        case vr_Float64_discrete_output:
            value[(*index)++] = M(Float64_discrete_output);
            return OK;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getInt8(ModelInstance* comp, ValueReference vr, int8_t *value, size_t *index) {
    
    UNUSED(value);
    UNUSED(index);
    
    calculateValues(comp);
        
    switch (vr) {
//        case vr_Float64_discrete_output:
//            value[(*index)++] = M(Int);
//            return OK;
        default:
            logError(comp, "Get Int8 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getInt32(ModelInstance* comp, ValueReference vr, int32_t *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_Int32_input:
            value[(*index)++] = M(Int32_input);
            return OK;
        case vr_Int32_output:
            value[(*index)++] = M(Int32_output);
            return OK;
        default:
            logError(comp, "Get Int32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_Boolean_input:
            value[(*index)++] = M(Boolean_input);
            return OK;
        case vr_Boolean_output:
            value[(*index)++] = M(Boolean_output);
            return OK;
        default:
            logError(comp, "Get Boolean is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getBinary(ModelInstance* comp, ValueReference vr, size_t size[], const char* value[], size_t* index) {

    calculateValues(comp);

    switch (vr) {
        case vr_Binary_input:
            value[*index]    = M(Binary_input);
            size[(*index)++] = M(Binary_input_size);
        case vr_Binary_output:
            value[*index]    = M(Binary_output);
            size[(*index)++] = M(Binary_output_size);
            return OK;
        default:
            logError(comp, "Get Binary is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index) {
    switch (vr) {
        case vr_String_parameter:
            value[(*index)++] = M(String_parameter);
            return OK;
        default:
            logError(comp, "Get String is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat32(ModelInstance* comp, ValueReference vr, const float value[], size_t *index) {
    switch (vr) {
        case vr_Float32_continuous_input:
            M(Float32_continuous_input) = value[(*index)++];
            return OK;
        case vr_Float32_discrete_input:
            if (comp->type == ModelExchange &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable Float32_discrete_input can only be set in initialization mode or event mode.");
                return Error;
            }
            M(Float32_discrete_input) = value[(*index)++];
            return OK;
        default:
            logError(comp, "Set Float32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {

        case vr_Float64_fixed_parameter:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode) {
                logError(comp, "Variable Float64_fixed_parameter can only be set after instantiation or in initialization mode.");
                return Error;
            }
#endif
            M(Float64_fixed_parameter) = value[(*index)++];
            return OK;

        case vr_Float64_tunable_parameter:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable Float64_tunable_parameter can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
#endif
            M(Float64_tunable_parameter) = value[(*index)++];
            return OK;

        case vr_Float64_continuous_input:
            M(Float64_continuous_input) = value[(*index)++];
            return OK;

        case vr_Float64_discrete_input:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable Float64_discrete_input can only be set in initialization mode or event mode.");
                return Error;
            }
#endif
            M(Float64_discrete_input) = value[(*index)++];
            return OK;

        default:
            logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {
    switch (vr) {
        case vr_Int32_input:
            M(Int32_input) = value[(*index)++];
            return OK;
        default:
            return Error;
    }
}

Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index) {
    switch (vr) {
        case vr_Boolean_input:
            M(Boolean_input) = value[(*index)++];
            return OK;
        case vr_Boolean_output:
            M(Boolean_output) = value[(*index)++];
            return OK;
        default:
            logError(comp, "Set Boolean is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index) {
    switch (vr) {
        case vr_String_parameter:
            if (strlen(value[*index]) >= STRING_MAX_LEN) {
                logError(comp, "Max. string length is %d bytes.", STRING_MAX_LEN);
                return Error;
            }
            strcpy(M(String_parameter), value[(*index)++]);
            return OK;
        default:
            logError(comp, "Set String is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char* const value[], size_t* index) {
    switch (vr) {
        case vr_Binary_input:
            if (size[*index] > BINARY_MAX_LEN) {
                logError(comp, "Max. binary size is %d bytes.", BINARY_MAX_LEN);
                return Error;
            }
            M(Binary_input_size) = size[*index];
            memcpy((void *)M(Binary_input), value[(*index)++], M(Binary_input_size));
            return OK;
        default:
            logError(comp, "Set Binary is not allowed for value reference %u.", vr);
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
