#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,continuous_real_out,discrete_real_out,int_out,bool_out\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

    const fmi3ValueReference float64ValueReferences[2] = { vr_continuous_real_out, vr_discrete_real_out };
    fmi3Float64 float64Values[2] = { 0 };
    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, float64ValueReferences, 2, float64Values, 2);

    const fmi3ValueReference int32ValueReferences[1] = { vr_int_out };
    fmi3Int32 int32Values[1] = { 0 };
    status = FMI3GetInt32((FMIInstance *)S, int32ValueReferences, 1, int32Values, 1);

    const fmi3ValueReference booleanValueReferences[1] = { vr_bool_out };
    fmi3Boolean booleanValues[1] = { 0 };
    status = FMI3GetBoolean((FMIInstance *)S, booleanValueReferences, 1, booleanValues, 1);

    fprintf(outputFile, "%g,%g,%g,%d,%d\n", ((FMIInstance *)S)->time, float64Values[0], float64Values[1], int32Values[0], booleanValues[0]);

    return status;
}