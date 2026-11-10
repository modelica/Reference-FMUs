#include "config.h"
#include "model_instance.h"
#include <stdlib.h>  // for free()
#include <string.h>  // for strcmp()

#ifdef _MSC_VER
#define strdup _strdup
#endif

#define STRING_START "Set me!"
#define BINARY_START "foo"

Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

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

    M(Int8_input)    = 0;
    M(Int8_output)   = 0;

    M(UInt8_input)   = 0;
    M(UInt8_output)  = 0;

    M(Int16_input)   = 0;
    M(Int16_output)  = 0;

    M(UInt16_input)  = 0;
    M(UInt16_output) = 0;

    M(Int32_input)   = 0;
    M(Int32_output)  = 0;

    M(UInt32_input)  = 0;
    M(UInt32_output) = 0;

    M(Int64_input)   = 0;
    M(Int64_output)  = 0;

    M(UInt64_input)  = 0;
    M(UInt64_output) = 0;

    M(Boolean_input)  = false;
    M(Boolean_output) = false;

    strncpy(M(String_input), STRING_START, STRING_MAX_LEN);
    strncpy(M(String_output), STRING_START, STRING_MAX_LEN);

    M(Binary_input_size) = strlen(BINARY_START);
    strncpy(M(Binary_input), BINARY_START, BINARY_MAX_LEN);
    M(Binary_output_size) = strlen(BINARY_START);
    strncpy(M(Binary_output), BINARY_START, BINARY_MAX_LEN);

    M(Enumeration_input)  = Option1;
    M(Enumeration_output) = Option1;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {

    M(Float32_continuous_output) = M(Float32_continuous_input);
    M(Float32_discrete_output)   = M(Float32_discrete_input) ;

    M(Float64_continuous_output) = M(Float64_continuous_input);
    M(Float64_discrete_output)   = M(Float64_discrete_input);

    M(Int8_output)    = M(Int8_input);

    M(UInt8_output)   = M(UInt8_input);

    M(Int16_output)   = M(Int16_input);

    M(UInt16_output)  = M(UInt16_input);

    M(Int32_output)   = M(Int32_input);

    M(UInt32_output)  = M(UInt32_input);

    M(Int64_output)   = M(Int64_input);

    M(UInt64_output)  = M(UInt64_input);

    M(Boolean_output) = M(Boolean_input);

    strncpy(M(String_output), M(String_input), STRING_MAX_LEN);

    M(Binary_output_size) = M(Binary_input_size);
    memcpy(M(Binary_output), M(Binary_input), M(Binary_input_size));

    M(Enumeration_output) = M(Enumeration_input);

    instance->isDirtyValues = false;

    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(size);

    if ((unsigned)vr <= (unsigned)vr_Enumeration_output) {
        *size = 1;
        return OK;
    }

    logError(instance, "Unknown value reference: %u.", vr);
    return Discard;
}

Status getFloat32(ModelInstance *instance, ValueReference vr, float values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Float32_continuous_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float32_continuous_input);
            break;
        case vr_Float32_continuous_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float32_continuous_output);
            break;
        case vr_Float32_discrete_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float32_discrete_input);
            break;
        case vr_Float32_discrete_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float32_discrete_output);
            break;
        default:
            logError(instance, "Get Float32 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_time:
            ASSERT_NVALUES(1);
            values[(*index)++] = instance->time;
            break;
        case vr_Float64_fixed_parameter:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float64_fixed_parameter);
            break;
        case vr_Float64_tunable_parameter:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float64_tunable_parameter);
            break;
        case vr_Float64_continuous_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float64_continuous_input);
            break;
        case vr_Float64_continuous_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float64_continuous_output);
            break;
        case vr_Float64_discrete_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float64_discrete_input);
            break;
        case vr_Float64_discrete_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Float64_discrete_output);
            break;
        default:
            logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getInt8(ModelInstance *instance, ValueReference vr, int8_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Int8_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int8_input);
            break;
        case vr_Int8_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int8_output);
            break;
        default:
            logError(instance, "Get Int8 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getUInt8(ModelInstance *instance, ValueReference vr, uint8_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_UInt8_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt8_input);
            break;
        case vr_UInt8_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt8_output);
            break;
        default:
            logError(instance, "Get UInt8 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getInt16(ModelInstance *instance, ValueReference vr, int16_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Int16_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int16_input);
            break;
        case vr_Int16_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int16_output);
            break;
        default:
            logError(instance, "Get Int16 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getUInt16(ModelInstance *instance, ValueReference vr, uint16_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_UInt16_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt16_input);
            break;
        case vr_UInt16_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt16_output);
            break;
        default:
            logError(instance, "Get UInt16 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getInt32(ModelInstance *instance, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Int32_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int32_input);
            break;
        case vr_Int32_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int32_output);
            break;
#if FMI_VERSION == 2
        case vr_Enumeration_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Enumeration_input);
            break;
        case vr_Enumeration_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Enumeration_output);
            break;
#endif
        default:
            logError(instance, "Get Int32 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getUInt32(ModelInstance *instance, ValueReference vr, uint32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_UInt32_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt32_input);
            break;
        case vr_UInt32_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt32_output);
            break;
        default:
            logError(instance, "Get UInt32 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getInt64(ModelInstance *instance, ValueReference vr, int64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Int64_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int64_input);
            break;
        case vr_Int64_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Int64_output);
            break;
#if FMI_VERSION == 3
        case vr_Enumeration_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Enumeration_input);
            break;
        case vr_Enumeration_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Enumeration_output);
            break;
#endif
        default:
            logError(instance, "Get Int64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getUInt64(ModelInstance *instance, ValueReference vr, uint64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_UInt64_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt64_input);
            break;
        case vr_UInt64_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(UInt64_output);
            break;
        default:
            logError(instance, "Get UInt64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getBoolean(ModelInstance *instance, ValueReference vr, bool values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Boolean_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Boolean_input);
            break;
        case vr_Boolean_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(Boolean_output);
            break;
        default:
            logError(instance, "Get Boolean is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getBinary(ModelInstance *instance, ValueReference vr, size_t sizes[], const char* values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(sizes);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_Binary_input:
            ASSERT_NVALUES(1);
            values[*index]    = M(Binary_input);
            sizes[(*index)++] = M(Binary_input_size);
            break;
        case vr_Binary_output:
            ASSERT_NVALUES(1);
            values[*index]    = M(Binary_output);
            sizes[(*index)++] = M(Binary_output_size);
            break;
        default:
            logError(instance, "Get Binary is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status getString(ModelInstance *instance, ValueReference vr, const char* values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_String_input:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(String_input);
            break;
        case vr_String_output:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(String_output);
            break;
        default:
            logError(instance, "Get String is not allowed for value reference %u.", vr);
            return Discard;
    }

    return OK;
}

Status setFloat32(ModelInstance *instance, ValueReference vr, const float values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Float32_continuous_input:
            ASSERT_NVALUES(1);
            M(Float32_continuous_input) = values[(*index)++];
            break;
        case vr_Float32_discrete_input:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != EventMode) {
                logError(instance, "Variable Float32_discrete_input can only be set in after instantiation, in Initialization Mode, and in Event Mode.");
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Float32_discrete_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set Float32 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {

        case vr_Float64_fixed_parameter:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode) {
                logError(instance, "Variable Float64_fixed_parameter can only be set after instantiation or in initialization mode.");
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Float64_fixed_parameter) = values[(*index)++];
            break;

        case vr_Float64_tunable_parameter:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != EventMode) {
                logError(instance, "Variable Float64_tunable_parameter can only be set after instantiation, in initialization mode or event mode.");
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Float64_tunable_parameter) = values[(*index)++];
            break;

        case vr_Float64_continuous_input:
            ASSERT_NVALUES(1);
            M(Float64_continuous_input) = values[(*index)++];
            break;

        case vr_Float64_discrete_input:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != EventMode) {
                logError(instance, "Variable Float64_discrete_input can only be set after instantiation, in initialization mode or event mode.");
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Float64_discrete_input) = values[(*index)++];
            break;

        default:
            logError(instance, "Set Float64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt8(ModelInstance *instance, ValueReference vr, const int8_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Int8_input:
            ASSERT_NVALUES(1);
            M(Int8_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set Int8 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt8(ModelInstance *instance, ValueReference vr, const uint8_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_UInt8_input:
            ASSERT_NVALUES(1);
            M(UInt8_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set UInt8 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt16(ModelInstance *instance, ValueReference vr, const int16_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Int16_input:
            ASSERT_NVALUES(1);
            M(Int16_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set Int16 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt16(ModelInstance *instance, ValueReference vr, const uint16_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_UInt16_input:
            ASSERT_NVALUES(1);
            M(UInt16_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set UInt16 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt32(ModelInstance *instance, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Int32_input:
            ASSERT_NVALUES(1);
            M(Int32_input) = values[(*index)++];
            break;
#if FMI_VERSION == 2
        case vr_Enumeration_input:
            if (values[*index] != Option1 && values[*index] != Option2) {
                logError(instance, "%d is not a legal value for Enumeration_input.", values[*index]);
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Enumeration_input) = values[(*index)++];
            break;
#endif
        default:
            logError(instance, "Set Int32 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt32(ModelInstance *instance, ValueReference vr, const uint32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_UInt32_input:
            ASSERT_NVALUES(1);
            M(UInt32_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set UInt32 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt64(ModelInstance *instance, ValueReference vr, const int64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Int64_input:
            ASSERT_NVALUES(1);
            M(Int64_input) = values[(*index)++];
            break;
#if FMI_VERSION == 3
        case vr_Enumeration_input:
            if (values[*index] != Option1 && values[*index] != Option2) {
                logError(instance, "%llu is not a legal value for Enumeration_input.", values[*index]);
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Enumeration_input) = values[(*index)++];
            break;
#endif
        default:
            logError(instance, "Set Int64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt64(ModelInstance *instance, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_UInt64_input:
            ASSERT_NVALUES(1);
            M(UInt64_input) = values[(*index)++];
            break;
        default:
            logError(instance, "Set UInt64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setBoolean(ModelInstance *instance, ValueReference vr, const bool values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Boolean_input:
            ASSERT_NVALUES(1);
            M(Boolean_input) = values[(*index)++];
            break;
        case vr_Boolean_output:
            ASSERT_NVALUES(1);
            M(Boolean_output) = values[(*index)++];
            break;
        default:
            logError(instance, "Set Boolean is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setString(ModelInstance *instance, ValueReference vr, const char *const values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_String_input:
            if (strlen(values[*index]) >= STRING_MAX_LEN) {
                logError(instance, "Max. string length is %d bytes.", STRING_MAX_LEN);
                return Discard;
            }
            ASSERT_NVALUES(1);
            strcpy(M(String_input), values[(*index)++]);
            break;
        default:
            logError(instance, "Set String is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setBinary(ModelInstance *instance, ValueReference vr, const size_t size[], const char* const values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(size);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_Binary_input:
            if (size[*index] > BINARY_MAX_LEN) {
                logError(instance, "Max. binary size is %d bytes.", BINARY_MAX_LEN);
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(Binary_input_size) = size[*index];
            memcpy((void *)M(Binary_input), values[(*index)++], M(Binary_input_size));
            break;
        default:
            logError(instance, "Set Binary is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}
