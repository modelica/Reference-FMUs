#define NO_INPUTS

#include "util.h"


FILE *createOutputFile(const char *outputFile) {

    FILE *file = fopen(outputFile, "w");

    if (file) {
        fputs("time,counter\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

#if FMI_VERSION == 2
    const fmi2ValueReference valueReferences[1] = { vr_counter };
    fmi2Integer values[1] = { 0 };
    FMIStatus status = FMI2GetInteger(S, valueReferences, 1, values);
#else
    const fmi3ValueReference valueReferences[1] = { vr_counter };
    fmi3Int32 values[1] = { 0 };
    FMIStatus status = FMI3GetInt32(S, valueReferences, 1, values, 1);
#endif

    fprintf(outputFile, "%g,%d\n", S->time, values[0]);

    return status;
}
