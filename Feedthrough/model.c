#include "config.h"
#include "model_impl.h"
#include <stdlib.h> // for free()
#include <string.h> // for strcmp()

#ifdef _MSC_VER
#define strdup _strdup
#endif

#define STRING_START "Set me!"
#define BINARY_START "foo"

Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    M(Float32_continuous_input) = 0.0f;
    M(Float32_continuous_output) = 0.0f;
    M(Float32_discrete_input) = 0.0f;
    M(Float32_discrete_output) = 0.0f;

    M(Float64_fixed_parameter) = 0.0;
    M(Float64_tunable_parameter) = 0.0;
    M(Float64_continuous_input) = 0.0;
    M(Float64_continuous_output) = 0.0;
    M(Float64_discrete_input) = 0.0;
    M(Float64_discrete_output) = 0.0;

    M(Int8_input) = 0;
    M(Int8_output) = 0;

    M(UInt8_input) = 0;
    M(UInt8_output) = 0;

    M(Int16_input) = 0;
    M(Int16_output) = 0;

    M(UInt16_input) = 0;
    M(UInt16_output) = 0;

    M(Int32_input) = 0;
    M(Int32_output) = 0;

    M(UInt32_input) = 0;
    M(UInt32_output) = 0;

    M(Int64_input) = 0;
    M(Int64_output) = 0;

    M(UInt64_input) = 0;
    M(UInt64_output) = 0;

    M(Boolean_input) = false;
    M(Boolean_output) = false;

    strncpy(M(String_input), STRING_START, STRING_MAX_LEN);
    strncpy(M(String_output), STRING_START, STRING_MAX_LEN);

    M(Binary_input_size) = strlen(BINARY_START);
    strncpy(M(Binary_input), BINARY_START, BINARY_MAX_LEN);
    M(Binary_output_size) = strlen(BINARY_START);
    strncpy(M(Binary_output), BINARY_START, BINARY_MAX_LEN);

    M(Enumeration_input) = Option1;
    M(Enumeration_output) = Option1;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {

    M(Float32_continuous_output) = M(Float32_continuous_input);
    M(Float32_discrete_output) = M(Float32_discrete_input);

    M(Float64_continuous_output) = M(Float64_continuous_input);
    M(Float64_discrete_output) = M(Float64_discrete_input);

    M(Int8_output) = M(Int8_input);

    M(UInt8_output) = M(UInt8_input);

    M(Int16_output) = M(Int16_input);

    M(UInt16_output) = M(UInt16_input);

    M(Int32_output) = M(Int32_input);

    M(UInt32_output) = M(UInt32_input);

    M(Int64_output) = M(Int64_input);

    M(UInt64_output) = M(UInt64_input);

    M(Boolean_output) = M(Boolean_input);

    strncpy(M(String_output), M(String_input), STRING_MAX_LEN);

    M(Binary_output_size) = M(Binary_input_size);
    memcpy(M(Binary_output), M(Binary_input), M(Binary_input_size));

    M(Enumeration_output) = M(Enumeration_input);

    instance->isDirtyValues = false;

    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);

    if ((unsigned)vr <= (unsigned)vr_Enumeration_output) {
        *size = 1;
        return OK;
    }

    logError(instance, "Unknown value reference: %u.", vr);
    return Discard;
}

Status getFloat32(ModelInstance *instance, ValueReference vr, float values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Float32_continuous_input:
        values[0] = M(Float32_continuous_input);
        return OK;
    case vr_Float32_continuous_output:
        values[0] = M(Float32_continuous_output);
        return OK;
    case vr_Float32_discrete_input:
        values[0] = M(Float32_discrete_input);
        return OK;
    case vr_Float32_discrete_output:
        values[0] = M(Float32_discrete_output);
        return OK;
    default:
        logError(instance, "Get Float32 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_time:
        values[0] = instance->time;
        return OK;
    case vr_Float64_fixed_parameter:
        values[0] = M(Float64_fixed_parameter);
        return OK;
    case vr_Float64_tunable_parameter:
        values[0] = M(Float64_tunable_parameter);
        return OK;
    case vr_Float64_continuous_input:
        values[0] = M(Float64_continuous_input);
        return OK;
    case vr_Float64_continuous_output:
        values[0] = M(Float64_continuous_output);
        return OK;
    case vr_Float64_discrete_input:
        values[0] = M(Float64_discrete_input);
        return OK;
    case vr_Float64_discrete_output:
        values[0] = M(Float64_discrete_output);
        return OK;
    default:
        logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInt8(ModelInstance *instance, ValueReference vr, int8_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Int8_input:
        values[0] = M(Int8_input);
        return OK;
    case vr_Int8_output:
        values[0] = M(Int8_output);
        return OK;
    default:
        logError(instance, "Get Int8 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getUInt8(ModelInstance *instance, ValueReference vr, uint8_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_UInt8_input:
        values[0] = M(UInt8_input);
        return OK;
    case vr_UInt8_output:
        values[0] = M(UInt8_output);
        return OK;
    default:
        logError(instance, "Get UInt8 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInt16(ModelInstance *instance, ValueReference vr, int16_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Int16_input:
        values[0] = M(Int16_input);
        return OK;
    case vr_Int16_output:
        values[0] = M(Int16_output);
        return OK;
    default:
        logError(instance, "Get Int16 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getUInt16(ModelInstance *instance, ValueReference vr, uint16_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_UInt16_input:
        values[0] = M(UInt16_input);
        return OK;
    case vr_UInt16_output:
        values[0] = M(UInt16_output);
        return OK;
    default:
        logError(instance, "Get UInt16 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInt32(ModelInstance *instance, ValueReference vr, int32_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Int32_input:
        values[0] = M(Int32_input);
        return OK;
    case vr_Int32_output:
        values[0] = M(Int32_output);
        return OK;
#if FMI_VERSION == 2
    case vr_Enumeration_input:
        values[0] = M(Enumeration_input);
        return OK;
    case vr_Enumeration_output:
        values[0] = M(Enumeration_output);
        return OK;
#endif
    default:
        logError(instance, "Get Int32 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getUInt32(ModelInstance *instance, ValueReference vr, uint32_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_UInt32_input:
        values[0] = M(UInt32_input);
        return OK;
    case vr_UInt32_output:
        values[0] = M(UInt32_output);
        return OK;
    default:
        logError(instance, "Get UInt32 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInt64(ModelInstance *instance, ValueReference vr, int64_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Int64_input:
        values[0] = M(Int64_input);
        return OK;
    case vr_Int64_output:
        values[0] = M(Int64_output);
        return OK;
#if FMI_VERSION == 3
    case vr_Enumeration_input:
        values[0] = M(Enumeration_input);
        return OK;
    case vr_Enumeration_output:
        values[0] = M(Enumeration_output);
        return OK;
#endif
    default:
        logError(instance, "Get Int64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getUInt64(ModelInstance *instance, ValueReference vr, uint64_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_UInt64_input:
        values[0] = M(UInt64_input);
        return OK;
    case vr_UInt64_output:
        values[0] = M(UInt64_output);
        return OK;
    default:
        logError(instance, "Get UInt64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getBoolean(ModelInstance *instance, ValueReference vr, bool values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Boolean_input:
        values[0] = M(Boolean_input);
        return OK;
    case vr_Boolean_output:
        values[0] = M(Boolean_output);
        return OK;
    default:
        logError(instance, "Get Boolean is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getBinary(ModelInstance *instance, ValueReference vr, size_t sizes[], const char *values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(sizes);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_Binary_input:
        values[0] = M(Binary_input);
        sizes[0] = M(Binary_input_size);
        return OK;
    case vr_Binary_output:
        values[0] = M(Binary_output);
        sizes[0] = M(Binary_output_size);
        return OK;
    default:
        logError(instance, "Get Binary is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getString(ModelInstance *instance, ValueReference vr, const char *values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    calculateValues(instance);

    switch (vr) {
    case vr_String_input:
        values[0] = M(String_input);
        return OK;
    case vr_String_output:
        values[0] = M(String_output);
        return OK;
    default:
        logError(instance, "Get String is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status setFloat32(ModelInstance *instance, ValueReference vr, const float values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Float32_continuous_input:
        M(Float32_continuous_input) = values[0];
        break;
    case vr_Float32_discrete_input:
        if (instance->type == ModelExchange &&
            instance->state != Instantiated &&
            instance->state != InitializationMode &&
            instance->state != EventMode) {
            logError(instance, "Variable Float32_discrete_input can only be set in after instantiation, in Initialization Mode, and in Event Mode.");
            return Discard;
        }
        M(Float32_discrete_input) = values[0];
        break;
    default:
        logError(instance, "Set Float32 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Float64_fixed_parameter:
        if (instance->type == ModelExchange &&
            instance->state != Instantiated &&
            instance->state != InitializationMode) {
            logError(instance, "Variable Float64_fixed_parameter can only be set after instantiation or in initialization mode.");
            return Discard;
        }
        M(Float64_fixed_parameter) = values[0];
        break;
    case vr_Float64_tunable_parameter:
        if (instance->type == ModelExchange &&
            instance->state != Instantiated &&
            instance->state != InitializationMode &&
            instance->state != EventMode) {
            logError(instance, "Variable Float64_tunable_parameter can only be set after instantiation, in initialization mode or event mode.");
            return Discard;
        }
        M(Float64_tunable_parameter) = values[0];
        break;
    case vr_Float64_continuous_input:
        M(Float64_continuous_input) = values[0];
        break;
    case vr_Float64_discrete_input:
        if (instance->type == ModelExchange &&
            instance->state != Instantiated &&
            instance->state != InitializationMode &&
            instance->state != EventMode) {
            logError(instance, "Variable Float64_discrete_input can only be set after instantiation, in initialization mode or event mode.");
            return Discard;
        }
        M(Float64_discrete_input) = values[0];
        break;
    default:
        logError(instance, "Set Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt8(ModelInstance *instance, ValueReference vr, const int8_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Int8_input:
        M(Int8_input) = values[0];
        break;
    default:
        logError(instance, "Set Int8 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt8(ModelInstance *instance, ValueReference vr, const uint8_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_UInt8_input:
        M(UInt8_input) = values[0];
        break;
    default:
        logError(instance, "Set UInt8 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt16(ModelInstance *instance, ValueReference vr, const int16_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Int16_input:
        M(Int16_input) = values[0];
        break;
    default:
        logError(instance, "Set Int16 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt16(ModelInstance *instance, ValueReference vr, const uint16_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_UInt16_input:
        M(UInt16_input) = values[0];
        break;
    default:
        logError(instance, "Set UInt16 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt32(ModelInstance *instance, ValueReference vr, const int32_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Int32_input:
        M(Int32_input) = values[0];
        break;
#if FMI_VERSION == 2
    case vr_Enumeration_input:
        if (values[0] != Option1 && values[0] != Option2) {
            logError(instance, "%d is not a legal value for Enumeration_input.", values[0]);
            return Discard;
        }
        M(Enumeration_input) = values[0];
        break;
#endif
    default:
        logError(instance, "Set Int32 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt32(ModelInstance *instance, ValueReference vr, const uint32_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_UInt32_input:
        M(UInt32_input) = values[0];
        break;
    default:
        logError(instance, "Set UInt32 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setInt64(ModelInstance *instance, ValueReference vr, const int64_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Int64_input:
        M(Int64_input) = values[0];
        break;
#if FMI_VERSION == 3
    case vr_Enumeration_input:
        if (values[0] != Option1 && values[0] != Option2) {
            logError(instance, "%llu is not a legal value for Enumeration_input.", values[0]);
            return Discard;
        }
        M(Enumeration_input) = values[0];
        break;
#endif
    default:
        logError(instance, "Set Int64 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setUInt64(ModelInstance *instance, ValueReference vr, const uint64_t values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_UInt64_input:
        M(UInt64_input) = values[0];
        break;
    default:
        logError(instance, "Set UInt64 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setBoolean(ModelInstance *instance, ValueReference vr, const bool values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Boolean_input:
        M(Boolean_input) = values[0];
        break;
    case vr_Boolean_output:
        M(Boolean_output) = values[0];
        break;
    default:
        logError(instance, "Set Boolean is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setString(ModelInstance *instance, ValueReference vr, const char *const values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_String_input:
        if (strlen(values[0]) >= STRING_MAX_LEN) {
            logError(instance, "Max. string length is %d bytes.", STRING_MAX_LEN);
            return Discard;
        }
        strcpy(M(String_input), values[0]);
        break;
    default:
        logError(instance, "Set String is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status setBinary(ModelInstance *instance, ValueReference vr, const size_t size[], const char *const values[], size_t nValues) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);
    ASSERT_NOT_NULL(values);
    ASSERT_SIZE_T(nValues, 1);

    switch (vr) {
    case vr_Binary_input:
        if (size[0] > BINARY_MAX_LEN) {
            logError(instance, "Max. binary size is %d bytes.", BINARY_MAX_LEN);
            return Discard;
        }
        M(Binary_input_size) = size[0];
        memcpy((void *)M(Binary_input), values[0], M(Binary_input_size));
        break;
    default:
        logError(instance, "Set Binary is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}
