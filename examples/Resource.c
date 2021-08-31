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

    fmi3Int32 values[1] = { 0 };

    FMIStatus status = FMI3GetInt32((FMIInstance *)S, valueReferences, 1, values, 1);

    fprintf(outputFile, "%g,%d\n", ((FMIInstance *)S)->time, values[0]);

    return status;
}
