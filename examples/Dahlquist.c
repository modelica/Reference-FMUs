#define NO_INPUTS

#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,x\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

#if FMI_VERSION == 1
    const fmi1ValueReference valueReferences[1] = { vr_x };
    fmi1Real values[1] = { 0 };
    FMIStatus status = FMI1GetReal((FMIInstance*)S, valueReferences, 1, values);
#elif FMI_VERSION == 2
    const fmi2ValueReference valueReferences[1] = { vr_x };
    fmi2Real values[1] = { 0 };
    FMIStatus status = FMI2GetReal((FMIInstance*)S, valueReferences, 1, values);
#elif FMI_VERSION == 3
    const fmi3ValueReference valueReferences[1] = { vr_x };
    fmi3Float64 values[1] = { 0 };
    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, valueReferences, 1, values, 1);
#endif

    fprintf(outputFile, "%g,%g\n", ((FMIInstance *)S)->time, values[0]);

    return status;
}
