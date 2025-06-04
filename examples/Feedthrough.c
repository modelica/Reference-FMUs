#include "util.h"
#include <limits.h>
#include <inttypes.h>


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
#if FMI_VERSION == 3
        fputs("time,Float32_continuous_output,Float32_discrete_output,Float64_continuous_output,Float64_discrete_output,Int8_output,UInt8_output,Int16_output,UInt16_output,Int32_output,UInt32_output,Int64_output,UInt64_output,Boolean_output,String_output,Binary_output\n", file);
#else
        fputs("time,Float64_continuous_output,Float64_discrete_output,Int32_output,Boolean_output,String_output\n", file);
#endif
    }

    return file;
}

double nextInputEventTime(double time) {
    if (time <= 0.5) {
        return 0.5;
    } else if (time <= 1.0) {
        return 1.0;
    } else if (time <= 1.5) {
        return 1.5;
    } else {
        return INFINITY;
    }
}

FMIStatus applyStartValues(FMIInstance *S) {

    FMIStatus status = FMIOK;

#if FMI_VERSION == 1
    const fmi1ValueReference float64ValueReferences[1] = { vr_Float64_fixed_parameter };
    const fmi1Real realValues[1] = { 1.0 };
    CALL(FMI1SetReal(S, float64ValueReferences, 1, realValues));
#elif FMI_VERSION == 2
    const fmi2ValueReference float64ValueReferences[1] = { vr_Float64_fixed_parameter };
    const fmi2Real realValues[1] = { 1.0 };
    CALL(FMI2SetReal(S, float64ValueReferences, 1, realValues));
#elif FMI_VERSION == 3
    const fmi3ValueReference float64ValueReferences[1] = { vr_Float64_fixed_parameter };
    const fmi3Float64 float64Values[1] = { 1.0 };
    CALL(FMI3SetFloat64(S, float64ValueReferences, 1, float64Values, 1));
#endif

TERMINATE:
    return status;
}

FMIStatus applyContinuousInputs(FMIInstance *S, double time, bool afterEvent) {

    FMIStatus status = FMIOK;

    double value;

    if (time < 0.5) {
        value = 0.0;
    } else if (time == 0.5) {
        value = afterEvent ? 2.0 : 0.0;
    } else if (time >= 0.5 && time < 1.0) {
        value = 2.0 - 2.0 * (time - 0.5);
    } else {
        value = 1.0;
    }

#if FMI_VERSION == 1
    const fmi1ValueReference valueReferences[1] = { vr_Float64_continuous_input };
    const fmi1Real values[1] = { value };
    CALL(FMI1SetReal(S, valueReferences, 1, values));
#elif FMI_VERSION == 2
    const fmi2ValueReference valueReferences[1] = { vr_Float64_continuous_input };
    const fmi2Real values[1] = { value };
    CALL(FMI2SetReal(S, valueReferences, 1, values));
#else
    const fmi3ValueReference Float32_vr[1] = { vr_Float32_continuous_input };
    const fmi3Float32 Float32_values[1] = { value };
    CALL(FMI3SetFloat32((FMIInstance *)S, Float32_vr, 1, Float32_values, 1));

    const fmi3ValueReference Float64_vr[1] = { vr_Float64_continuous_input };
    const fmi3Float64 Float64_values[1] = { value };
    CALL(FMI3SetFloat64((FMIInstance *)S, Float64_vr, 1, Float64_values, 1));
#endif

TERMINATE:
    return status;
}

FMIStatus applyDiscreteInputs(FMIInstance *S, double time) {

    FMIStatus status = FMIOK;

    const bool before_step = time < 1.0;

#if FMI_VERSION == 1
    const fmi1ValueReference float64ValueReferences[2] = { vr_Float64_tunable_parameter, vr_Float64_discrete_input };
    const fmi1Real float64Values[2] = { time < 1.5 ? 0.0 : -1.0, before_step ? 0 : 1.0 };
    CALL(FMI1SetReal(S, float64ValueReferences, 1, float64Values));

    const fmi1ValueReference int32ValueReferences[1] = { vr_Int32_input };
    const fmi1Integer int32Values[1] = { before_step ? INT32_MIN : INT32_MAX };
    CALL(FMI1SetInteger(S, int32ValueReferences, 1, int32Values));

    const fmi1ValueReference booleanValueReferences[1] = { vr_Boolean_input };
    const fmi1Boolean booleanValues[1] = { before_step ? false : true };
    CALL(FMI1SetBoolean(S, booleanValueReferences, 1, booleanValues));

    const fmi1ValueReference stringValueReferences[1] = { vr_String_input };
    const fmi1String stringValues[1] = { before_step ? "FMI is awesome!" : "FMI is still awesome!" };
    CALL(FMI1SetString(S, stringValueReferences, 1, stringValues));
#elif FMI_VERSION == 2
    const fmi2ValueReference float64ValueReferences[2] = { vr_Float64_tunable_parameter, vr_Float64_discrete_input };
    const fmi2Real float64Values[2] = { time < 1.5 ? 0.0 : -1.0, before_step ? 0 : 1.0 };
    CALL(FMI2SetReal(S, float64ValueReferences, 1, float64Values));

    const fmi2ValueReference int32ValueReferences[1] = { vr_Int32_input };
    const fmi2Integer int32Values[1] = { before_step ? INT32_MIN : INT32_MAX };
    CALL(FMI2SetInteger(S, int32ValueReferences, 1, int32Values));

    const fmi2ValueReference booleanValueReferences[1] = { vr_Boolean_input };
    const fmi2Boolean booleanValues[1] = { before_step ? false : true };
    CALL(FMI2SetBoolean(S, booleanValueReferences, 1, booleanValues));

    const fmi2ValueReference stringValueReferences[1] = { vr_String_input };
    const fmi2String stringValues[1] = { before_step ? "FMI is awesome!" : "FMI is still awesome!" };
    CALL(FMI2SetString(S, stringValueReferences, 1, stringValues));
#elif FMI_VERSION == 3
    const fmi3ValueReference Float32_vr[1] = { vr_Float32_discrete_input };
    const fmi3Float32 Float32_values[1] = { before_step ? 0 : 1.0f };
    CALL(FMI3SetFloat32(S, Float32_vr, 1, Float32_values, 1));

    const fmi3ValueReference Float64_vr[2] = { vr_Float64_tunable_parameter, vr_Float64_discrete_input };
    const fmi3Float64 Float64_values[2] = { time < 1.5 ? 0.0 : -1.0, before_step ? 0 : 1.0 };
    CALL(FMI3SetFloat64(S, Float64_vr, 2, Float64_values, 2));

    const fmi3ValueReference Int8_vr[1] = { vr_Int8_input };
    const fmi3Int8 Int8_values[1] = { before_step ? INT8_MIN : INT8_MAX };
    CALL(FMI3SetInt8(S, Int8_vr, 1, Int8_values, 1));

    const fmi3ValueReference UInt8_vr[1] = { vr_UInt8_input };
    const fmi3UInt8 UInt8_values[1] = { before_step ? 0 : UINT8_MAX };
    CALL(FMI3SetUInt8(S, UInt8_vr, 1, UInt8_values, 1));

    const fmi3ValueReference Int16_vr[1] = { vr_Int16_input };
    const fmi3Int16 Int16_values[1] = { before_step ? INT16_MIN : INT16_MAX };
    CALL(FMI3SetInt16(S, Int16_vr, 1, Int16_values, 1));

    const fmi3ValueReference UInt16_vr[1] = { vr_UInt16_input };
    const fmi3UInt16 UInt16_values[1] = { before_step ? 0 : UINT16_MAX };
    CALL(FMI3SetUInt16(S, UInt16_vr, 1, UInt16_values, 1));

    const fmi3ValueReference Int32_vr[1] = { vr_Int32_input };
    const fmi3Int32 Int32_values[1] = { before_step ? INT32_MIN : INT32_MAX };
    CALL(FMI3SetInt32(S, Int32_vr, 1, Int32_values, 1));

    const fmi3ValueReference UInt32_vr[1] = { vr_UInt32_input };
    const fmi3UInt32 UInt32_values[1] = { before_step ? 0 : UINT32_MAX };
    CALL(FMI3SetUInt32(S, UInt32_vr, 1, UInt32_values, 1));

    const fmi3ValueReference Int64_vr[1] = { vr_Int64_input };
    const fmi3Int64 Int64_values[1] = { before_step ? INT64_MIN : INT64_MAX };
    CALL(FMI3SetInt64(S, Int64_vr, 1, Int64_values, 1));

    const fmi3ValueReference UInt64_vr[1] = { vr_UInt64_input };
    const fmi3UInt64 UInt64_values[1] = { before_step ? 0 : UINT64_MAX };
    CALL(FMI3SetUInt64(S, UInt64_vr, 1, UInt64_values, 1));

    const fmi3ValueReference booleanValueReferences[1] = { vr_Boolean_input };
    const fmi3Boolean booleanValues[1] = { before_step ? false : true };
    CALL(FMI3SetBoolean(S, booleanValueReferences, 1, booleanValues, 1));

    const fmi3ValueReference stringValueReferences[1] = { vr_String_input };
    const fmi3String stringValues[1] = { before_step ? "FMI is awesome!" : "FMI is still awesome!" };
    CALL(FMI3SetString(S, stringValueReferences, 1, stringValues, 1));

    const fmi3ValueReference binaryValueReferences[1] = { vr_Binary_input };
    const fmi3Binary binaryValues[1] = { before_step ? (void*)"foo" : (void*)"bar" };
    const size_t binarySizes[1] = { 3 };
    CALL(FMI3SetBinary(S, binaryValueReferences, 1, binarySizes, binaryValues, 1));
#endif

TERMINATE:
    return status;
}

FMIStatus recordVariables(FMIInstance *S, double time, FILE *outputFile) {

    FMIStatus status = FMIOK;

#if FMI_VERSION == 1

    const fmi1ValueReference float64ValueReferences[2] = { vr_Float64_continuous_output, vr_Float64_discrete_output };
    fmi1Real Float64_values[2] = { 0 };
    CALL(FMI1GetReal(S, float64ValueReferences, 2, Float64_values));

    const fmi1ValueReference Int32_vr[1] = { vr_Int32_output };
    fmi1Integer Int32_values[1] = { 0 };
    CALL(FMI1GetInteger(S, Int32_vr, 1, Int32_values));

    const fmi1ValueReference Boolean_vr[1] = { vr_Boolean_output };
    fmi1Boolean Boolean_values[1] = { 0 };
    CALL(FMI1GetBoolean(S, Boolean_vr, 1, Boolean_values));

    const fmi1ValueReference String_vr[1] = { vr_String_output };
    fmi1String String_values[1] = { NULL };
    CALL(FMI1GetString((FMIInstance*)S, String_vr, 1, String_values));

    fprintf(outputFile, "%g,%.16g,%.16g,%" PRIu32 ",%d,\"%s\"\n",
        time, Float64_values[0], Float64_values[1], Int32_values[0], Boolean_values[0], String_values[0]);

#elif FMI_VERSION == 2

    const fmi2ValueReference float64ValueReferences[2] = { vr_Float64_continuous_output, vr_Float64_discrete_output };
    fmi2Real Float64_values[2] = { 0 };
    CALL(FMI2GetReal(S, float64ValueReferences, 2, Float64_values));

    const fmi2ValueReference Int32_vr[1] = { vr_Int32_output };
    fmi2Integer Int32_values[1] = { 0 };
    CALL(FMI2GetInteger(S, Int32_vr, 1, Int32_values));

    const fmi2ValueReference Boolean_vr[1] = { vr_Boolean_output };
    fmi2Boolean Boolean_values[1] = { 0 };
    CALL(FMI2GetBoolean(S, Boolean_vr, 1, Boolean_values));

    const fmi2ValueReference String_vr[1] = { vr_String_output };
    fmi2String String_values[1] = { NULL };
    CALL(FMI2GetString((FMIInstance*)S, String_vr, 1, String_values));

    fprintf(outputFile, "%g,%.16g,%.16g,%" PRIu32 ",%d,\"%s\"\n",
        time, Float64_values[0], Float64_values[1], Int32_values[0], Boolean_values[0], String_values[0]);

#elif FMI_VERSION == 3

    const fmi3ValueReference Float32_vr[2] = { vr_Float32_continuous_output, vr_Float32_discrete_output };
    fmi3Float32 Float32_values[2] = { 0 };
    CALL(FMI3GetFloat32((FMIInstance *)S, Float32_vr, 2, Float32_values, 2));

    const fmi3ValueReference Float64_vr[2] = { vr_Float64_continuous_output, vr_Float64_discrete_output };
    fmi3Float64 Float64_values[2] = { 0 };
    CALL(FMI3GetFloat64((FMIInstance *)S, Float64_vr, 2, Float64_values, 2));

    const fmi3ValueReference Int8_vr[1] = { vr_Int8_output };
    fmi3Int8 Int8_values[1] = { 0 };
    CALL(FMI3GetInt8((FMIInstance *)S, Int8_vr, 1, Int8_values, 1));

    const fmi3ValueReference UInt8_vr[1] = { vr_UInt8_output };
    fmi3UInt8 UInt8_values[1] = { 0 };
    CALL(FMI3GetUInt8((FMIInstance *)S, UInt8_vr, 1, UInt8_values, 1));

    const fmi3ValueReference Int16_vr[1] = { vr_Int16_output };
    fmi3Int16 Int16_values[1] = { 0 };
    CALL(FMI3GetInt16((FMIInstance *)S, Int16_vr, 1, Int16_values, 1));

    const fmi3ValueReference UInt16_vr[1] = { vr_UInt16_output };
    fmi3UInt16 UInt16_values[1] = { 0 };
    CALL(FMI3GetUInt16((FMIInstance *)S, UInt16_vr, 1, UInt16_values, 1));

    const fmi3ValueReference Int32_vr[1] = { vr_Int32_output };
    fmi3Int32 Int32_values[1] = { 0 };
    CALL(FMI3GetInt32((FMIInstance *)S, Int32_vr, 1, Int32_values, 1));

    const fmi3ValueReference UInt32_vr[1] = { vr_UInt32_output };
    fmi3UInt32 UInt32_values[1] = { 0 };
    CALL(FMI3GetUInt32((FMIInstance *)S, UInt32_vr, 1, UInt32_values, 1));

    const fmi3ValueReference Int64_vr[1] = { vr_Int64_output };
    fmi3Int64 Int64_values[1] = { 0 };
    CALL(FMI3GetInt64((FMIInstance *)S, Int64_vr, 1, Int64_values, 1));

    const fmi3ValueReference UInt64_vr[1] = { vr_UInt64_output };
    fmi3UInt64 UInt64_values[1] = { 0 };
    CALL(FMI3GetUInt64((FMIInstance *)S, UInt64_vr, 1, UInt64_values, 1));

    const fmi3ValueReference Boolean_vr[1] = { vr_Boolean_output };
    fmi3Boolean Boolean_values[1] = { 0 };
    CALL(FMI3GetBoolean((FMIInstance *)S, Boolean_vr, 1, Boolean_values, 1));

    const fmi3ValueReference String_vr[1] = { vr_String_output };
    fmi3String String_values[1] = { NULL };
    CALL(FMI3GetString((FMIInstance*)S, String_vr, 1, String_values, 1));

    const fmi3ValueReference Binary_vr[1] = { vr_Binary_output };
    size_t Binary_sizes[1] = { 0 };
    fmi3Binary Binary_values[1] = { NULL };
    CALL(FMI3GetBinary((FMIInstance*)S, Binary_vr, 1, Binary_sizes, Binary_values, 1));

    fprintf(outputFile, "%g,%.7g,%.7g,%.16g,%.16g,%" PRId8 ",%" PRIu8 ",%" PRId16 ",%" PRIu16 ",%" PRId32 ",%" PRIu32 ",%" PRId64 ",%" PRIu64 ",%d,\"%s\",",
        time, Float32_values[0], Float32_values[1], Float64_values[0], Float64_values[1], Int8_values[0], UInt8_values[0], Int16_values[0], UInt16_values[0], Int32_values[0], UInt32_values[0], Int64_values[0], UInt64_values[0], Boolean_values[0], String_values[0]);

    for (size_t i = 0; i < Binary_sizes[0]; i++) {

        const fmi3Byte b = Binary_values[0][i];

        const char hex[3] = {
            "0123456789abcdef"[b >> 4],
            "0123456789abcdef"[b & 0x0F],
            '\0'
        };

        fputs(hex, outputFile);
    }

    fputc('\n', outputFile);

#endif

TERMINATE:
    return status;
}
