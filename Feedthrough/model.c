#include "config.h"
#include "model.h"
#include <stdlib.h>  // for free()
#include <string.h>  // for strcmp()

#ifdef _MSC_VER
#define strdup _strdup
#endif

#define STRING_START "Set me!"
#define BINARY_START "foo"

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

    strncpy(M(String_parameter), STRING_START, STRING_MAX_LEN);

    M(Binary_input_size) = strlen(BINARY_START);
    strncpy(M(Binary_input), BINARY_START, BINARY_MAX_LEN);
    M(Binary_output_size) = strlen(BINARY_START);
    strncpy(M(Binary_output), BINARY_START, BINARY_MAX_LEN);

    M(Enumeration_input)  = Option1;
    M(Enumeration_output) = Option1;
}

Status calculateValues(ModelInstance *comp) {

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

    M(Binary_output_size) = M(Binary_input_size);
    memcpy(M(Binary_output), M(Binary_input), M(Binary_input_size));

    M(Enumeration_output) = M(Enumeration_input);

    return OK;
}

Status getFloat32(ModelInstance* comp, ValueReference vr, float values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Float32_continuous_input:
            values[(*index)++] = M(Float32_continuous_input);
            break;
        case vr_Float32_continuous_output:
            values[(*index)++] = M(Float32_continuous_output);
            break;
        case vr_Float32_discrete_input:
            values[(*index)++] = M(Float32_discrete_input);
            break;
        case vr_Float32_discrete_output:
            values[(*index)++] = M(Float32_discrete_output);
            break;
        default:
            logError(comp, "Get Float32 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_time:
            values[(*index)++] = comp->time;
            break;
        case vr_Float64_fixed_parameter:
            values[(*index)++] = M(Float64_fixed_parameter);
            break;
        case vr_Float64_tunable_parameter:
            values[(*index)++] = M(Float64_tunable_parameter);
            break;
        case vr_Float64_continuous_input:
            values[(*index)++] = M(Float64_continuous_input);
            break;
        case vr_Float64_continuous_output:
            values[(*index)++] = M(Float64_continuous_output);
            break;
        case vr_Float64_discrete_input:
            values[(*index)++] = M(Float64_discrete_input);
            break;
        case vr_Float64_discrete_output:
            values[(*index)++] = M(Float64_discrete_output);
            break;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getInt8(ModelInstance* comp, ValueReference vr, int8_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int8_input:
            values[(*index)++] = M(Int8_input);
            break;
        case vr_Int8_output:
            values[(*index)++] = M(Int8_output);
            break;
        default:
            logError(comp, "Get Int8 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getUInt8(ModelInstance* comp, ValueReference vr, uint8_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt8_input:
            values[(*index)++] = M(UInt8_input);
            break;
        case vr_UInt8_output:
            values[(*index)++] = M(UInt8_output);
            break;
        default:
            logError(comp, "Get UInt8 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getInt16(ModelInstance* comp, ValueReference vr, int16_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int16_input:
            values[(*index)++] = M(Int16_input);
            break;
        case vr_Int16_output:
            values[(*index)++] = M(Int16_output);
            break;
        default:
            logError(comp, "Get Int16 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getUInt16(ModelInstance* comp, ValueReference vr, uint16_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt16_input:
            values[(*index)++] = M(UInt16_input);
            break;
        case vr_UInt16_output:
            values[(*index)++] = M(UInt16_output);
            break;
        default:
            logError(comp, "Get UInt16 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getInt32(ModelInstance* comp, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int32_input:
            values[(*index)++] = M(Int32_input);
            break;
        case vr_Int32_output:
            values[(*index)++] = M(Int32_output);
            break;
#if FMI_VERSION == 1 || FMI_VERSION == 2
        case vr_Enumeration_input:
            values[(*index)++] = M(Enumeration_input);
            break;
        case vr_Enumeration_output:
            values[(*index)++] = M(Enumeration_output);
            break;
#endif
        default:
            logError(comp, "Get Int32 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getUInt32(ModelInstance* comp, ValueReference vr, uint32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt32_input:
            values[(*index)++] = M(UInt32_input);
            break;
        case vr_UInt32_output:
            values[(*index)++] = M(UInt32_output);
            break;
        default:
            logError(comp, "Get UInt32 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getInt64(ModelInstance* comp, ValueReference vr, int64_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int64_input:
            values[(*index)++] = M(Int64_input);
            break;
        case vr_Int64_output:
            values[(*index)++] = M(Int64_output);
            break;
#if FMI_VERSION == 3
        case vr_Enumeration_input:
            values[(*index)++] = M(Enumeration_input);
            break;
        case vr_Enumeration_output:
            values[(*index)++] = M(Enumeration_output);
            break;
#endif
        default:
            logError(comp, "Get Int64 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getUInt64(ModelInstance* comp, ValueReference vr, uint64_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt64_input:
            values[(*index)++] = M(UInt64_input);
            break;
        case vr_UInt64_output:
            values[(*index)++] = M(UInt64_output);
            break;
        default:
            logError(comp, "Get UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getBoolean(ModelInstance* comp, ValueReference vr, bool values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Boolean_input:
            values[(*index)++] = M(Boolean_input);
            break;
        case vr_Boolean_output:
            values[(*index)++] = M(Boolean_output);
            break;
        default:
            logError(comp, "Get Boolean is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getBinary(ModelInstance* comp, ValueReference vr, size_t sizes[], const char* values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Binary_input:
            values[*index]    = M(Binary_input);
            sizes[(*index)++] = M(Binary_input_size);
            break;
        case vr_Binary_output:
            values[*index]    = M(Binary_output);
            sizes[(*index)++] = M(Binary_output_size);
            break;
        default:
            logError(comp, "Get Binary is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status getString(ModelInstance* comp, ValueReference vr, const char* values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_String_parameter:
            values[(*index)++] = M(String_parameter);
            break;
        default:
            logError(comp, "Get String is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status setFloat32(ModelInstance* comp, ValueReference vr, const float values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Float32_continuous_input:
            M(Float32_continuous_input) = values[(*index)++];
            break;
        case vr_Float32_discrete_input:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable Float32_discrete_input can only be set in after instantiation, in Initialization Mode, and in Event Mode.");
                return Error;
            }
#endif
            M(Float32_discrete_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set Float32 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

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
            M(Float64_fixed_parameter) = values[(*index)++];
            break;

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
            M(Float64_tunable_parameter) = values[(*index)++];
            break;

        case vr_Float64_continuous_input:
            M(Float64_continuous_input) = values[(*index)++];
            break;

        case vr_Float64_discrete_input:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable Float64_discrete_input can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
#endif
            M(Float64_discrete_input) = values[(*index)++];
            break;

        default:
            logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status setInt8(ModelInstance* comp, ValueReference vr, const int8_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int8_input:
            M(Int8_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set Int8 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status setUInt8(ModelInstance* comp, ValueReference vr, const uint8_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt8_input:
            M(UInt8_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set UInt8 is not allowed for value reference %u.", vr);
            return Error;
    }

    return OK;
}

Status setInt16(ModelInstance* comp, ValueReference vr, const int16_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int16_input:
            M(Int16_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set Int16 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setUInt16(ModelInstance* comp, ValueReference vr, const uint16_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt16_input:
            M(UInt16_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set UInt16 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int32_input:
            M(Int32_input) = values[(*index)++];
            break;
#if FMI_VERSION == 1 || FMI_VERSION == 2
        case vr_Enumeration_input:
            if (values[*index] != Option1 && values[*index] != Option2) {
                logError(comp, "%d is not a legal value for Enumeration_input.", values[*index]);
                return Error;
            }
            M(Enumeration_input) = values[(*index)++];
            break;
#endif
        default:
            logError(comp, "Set Int32 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setUInt32(ModelInstance* comp, ValueReference vr, const uint32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt32_input:
            M(UInt32_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set UInt32 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setInt64(ModelInstance* comp, ValueReference vr, const int64_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Int64_input:
            M(Int64_input) = values[(*index)++];
            break;
#if FMI_VERSION == 3
        case vr_Enumeration_input:
            if (values[*index] != Option1 && values[*index] != Option2) {
                logError(comp, "%llu is not a legal value for Enumeration_input.", values[*index]);
                return Error;
            }
            M(Enumeration_input) = values[(*index)++];
            break;
#endif
        default:
            logError(comp, "Set Int64 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setUInt64(ModelInstance* comp, ValueReference vr, const uint64_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_UInt64_input:
            M(UInt64_input) = values[(*index)++];
            break;
        default:
            logError(comp, "Set UInt64 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setBoolean(ModelInstance* comp, ValueReference vr, const bool values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Boolean_input:
            M(Boolean_input) = values[(*index)++];
            break;
        case vr_Boolean_output:
            M(Boolean_output) = values[(*index)++];
            break;
        default:
            logError(comp, "Set Boolean is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setString(ModelInstance* comp, ValueReference vr, const char *const values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_String_parameter:
            if (strlen(values[*index]) >= STRING_MAX_LEN) {
                logError(comp, "Max. string length is %d bytes.", STRING_MAX_LEN);
                return Error;
            }
            strcpy(M(String_parameter), values[(*index)++]);
            break;
        default:
            logError(comp, "Set String is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status setBinary(ModelInstance* comp, ValueReference vr, const size_t size[], const char* const values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_Binary_input:
            if (size[*index] > BINARY_MAX_LEN) {
                logError(comp, "Max. binary size is %d bytes.", BINARY_MAX_LEN);
                return Error;
            }
            M(Binary_input_size) = size[*index];
            memcpy((void *)M(Binary_input), values[(*index)++], M(Binary_input_size));
            break;
        default:
            logError(comp, "Set Binary is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
    return OK;
}
