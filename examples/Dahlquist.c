#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,x\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

    const fmi3ValueReference valueReferences[1] = { vr_x };

    fmi3Float64 values[1] = { 0 };

    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, valueReferences, 1, values, 1);

    fprintf(outputFile, "%g,%g\n", ((FMIInstance *)S)->time, values[0]);

    return status;
}
