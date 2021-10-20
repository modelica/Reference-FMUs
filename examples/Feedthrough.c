#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,continuous_real_out,discrete_real_out,int_out,bool_out\n", file);
    }

    return file;
}

double nextInputEventTime(double time) {
    if (time <= 0.5) {
        return 0.5;
    } else if (time <= 1.0) {
        return 1.0;
    } else if (time <= 1.5) {
        return 1.0;
    } else {
        return INFINITY;
    }
}

FMIStatus applyStartValues(FMIInstance *S) {

#if FMI_VERSION == 2
    const fmi2ValueReference float64ValueReferences[1] = { vr_fixed_real_parameter };
    const fmi2Real realValues[1] = { 1.0 };
    FMI2SetReal(S, float64ValueReferences, 1, realValues);

    const fmi2ValueReference stringValueReferences[1] = { vr_string };
    const fmi2String stringValues[1] = { "FMI is awesome!" };
    FMI2SetString(S, stringValueReferences, 1, stringValues);
#else
    const fmi3ValueReference float64ValueReferences[1] = { vr_fixed_real_parameter };
    const fmi3Float64 float64Values[1] = { 1.0 };
    FMI3SetFloat64(S, float64ValueReferences, 1, float64Values, 1);

    const fmi3ValueReference stringValueReferences[1] = { vr_string };
    const fmi3String stringValues[1] = { "FMI is awesome!" };
    FMI3SetString(S, stringValueReferences, 1, stringValues, 1);
#endif

    return FMIOK;
}

FMIStatus applyContinuousInputs(FMIInstance *S, bool afterEvent) {

#if FMI_VERSION == 2
    const fmi2ValueReference valueReferences[1] = { vr_continuous_real_in };
    fmi2Real values[1];
#else
    const fmi3ValueReference valueReferences[1] = { vr_continuous_real_in };
    fmi3Float64 values[1];
#endif

    if (S->time < 0.5) {
        values[0] = 0.0;
    } else if (S->time == 0.5) {
        values[0] = afterEvent ? 2.0 : 0.0;
    } else if (S->time >= 0.5 && S->time < 1.0) {
        values[0] = 2.0 - 2.0 * (S->time - 0.5);
    } else {
        values[0] = 1.0;
    }

#if FMI_VERSION == 2
    return FMI2SetReal((FMIInstance *)S, valueReferences, 1, values);
#else
    return FMI3SetFloat64((FMIInstance *)S, valueReferences, 1, values, 1);
#endif
}

FMIStatus applyDiscreteInputs(FMIInstance *S) {

#if FMI_VERSION == 2
    const fmi2ValueReference float64ValueReferences[2] = { vr_tunable_real_parameter, vr_discrete_real_in };
    const fmi2Real float64Values[2] = { S->time < 1.5 ? 0.0 : -1.0, S->time < 1.0 ? 0 : 1.0 };
    FMI2SetReal(S, float64ValueReferences, 1, float64Values);

    const fmi2ValueReference int32ValueReferences[1] = { vr_int_in };
    const fmi2Integer int32Values[1] = { S->time < 1.0 ? 0 : 1 };
    FMI2SetInteger(S, int32ValueReferences, 1, int32Values);

    const fmi2ValueReference booleanValueReferences[1] = { vr_bool_in };
    const fmi2Boolean booleanValues[1] = { S->time < 1.0 ? false : true };
    FMI2SetBoolean(S, booleanValueReferences, 1, booleanValues);
#else
    const fmi3ValueReference float64ValueReferences[2] = { vr_tunable_real_parameter, vr_discrete_real_in };
    const fmi3Float64 float64Values[2] = { S->time < 1.5 ? 0.0 : -1.0, S->time < 1.0 ? 0 : 1.0 };
    FMI3SetFloat64(S, float64ValueReferences, 1, float64Values, 1);

    const fmi3ValueReference int32ValueReferences[1] = { vr_int_in };
    const fmi3Int32 int32Values[1] = { S->time < 1.0 ? 0 : 1 };
    FMI3SetInt32(S, int32ValueReferences, 1, int32Values, 1);

    const fmi3ValueReference booleanValueReferences[1] = { vr_bool_in };
    const fmi3Boolean booleanValues[1] = { S->time < 1.0 ? false : true };
    FMI3SetBoolean(S, booleanValueReferences, 1, booleanValues, 1);
#endif

    return FMIOK;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

#if FMI_VERSION == 2
    const fmi2ValueReference float64ValueReferences[2] = { vr_continuous_real_out, vr_discrete_real_out };
    fmi2Real float64Values[2] = { 0 };
    FMIStatus status = FMI2GetReal((FMIInstance *)S, float64ValueReferences, 2, float64Values);

    const fmi2ValueReference int32ValueReferences[1] = { vr_int_out };
    fmi2Integer int32Values[1] = { 0 };
    status = FMI2GetInteger((FMIInstance *)S, int32ValueReferences, 1, int32Values);

    const fmi2ValueReference booleanValueReferences[1] = { vr_bool_out };
    fmi2Boolean booleanValues[1] = { 0 };
    status = FMI2GetBoolean((FMIInstance *)S, booleanValueReferences, 1, booleanValues);
#else
    const fmi3ValueReference float64ValueReferences[2] = { vr_continuous_real_out, vr_discrete_real_out };
    fmi3Float64 float64Values[2] = { 0 };
    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, float64ValueReferences, 2, float64Values, 2);

    const fmi3ValueReference int32ValueReferences[1] = { vr_int_out };
    fmi3Int32 int32Values[1] = { 0 };
    status = FMI3GetInt32((FMIInstance *)S, int32ValueReferences, 1, int32Values, 1);

    const fmi3ValueReference booleanValueReferences[1] = { vr_bool_out };
    fmi3Boolean booleanValues[1] = { 0 };
    status = FMI3GetBoolean((FMIInstance *)S, booleanValueReferences, 1, booleanValues, 1);
#endif

    fprintf(outputFile, "%g,%g,%g,%d,%d\n", ((FMIInstance *)S)->time, float64Values[0], float64Values[1], int32Values[0], booleanValues[0]);

    return status;
}