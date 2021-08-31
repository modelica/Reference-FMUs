#define NO_INPUTS

#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,x0,x1\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

    const fmi3ValueReference valueReferences[2] = { vr_x0, vr_x1 };

    fmi3Float64 values[2] = { 0 };

    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, valueReferences, 2, values, 2);

    fprintf(outputFile, "%g,%g,%g\n", ((FMIInstance *)S)->time, values[0], values[1]);

    return status;
}
