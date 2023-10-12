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

    const fmi3ValueReference valueReferences[1] = { vr_y };

    fmi3Float64 values[3] = { 0 };

    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, valueReferences, 1, values, 3);

    fprintf(outputFile, "%g,%g %g %g\n", ((FMIInstance *)S)->time, values[0], values[1], values[2]);

    return status;
}
