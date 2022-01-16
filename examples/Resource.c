#define NO_INPUTS

#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,y\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

#if FMI_VERSION == 2
    const fmi2ValueReference valueReferences[1] = { vr_y };
    fmi2Integer values[1] = { 0 };
    FMIStatus status = FMI2GetInteger((FMIInstance *)S, valueReferences, 1, values);
#else
    const fmi3ValueReference valueReferences[1] = { vr_y };
    fmi3Int32 values[1] = { 0 };
    FMIStatus status = FMI3GetInt32((FMIInstance *)S, valueReferences, 1, values, 1);
#endif

    fprintf(outputFile, "%g,%d\n", ((FMIInstance *)S)->time, values[0]);

    return status;
}
