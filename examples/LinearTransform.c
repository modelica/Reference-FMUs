#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,y[1],y[2]\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, FILE *outputFile) {

    const fmi3ValueReference valueReferences[1] = { vr_y };

    fmi3Float64 values[2] = { 0 };

    FMIStatus status = FMI3GetFloat64((FMIInstance *)S, valueReferences, 1, values, 2);

    fprintf(outputFile, "%g,%g,%g\n", ((FMIInstance *)S)->time, values[0], values[1]);

    return status;
}
