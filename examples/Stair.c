#include <stdio.h>
#include "FMU.h"
#include "config.h"


FILE *openOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,counter\n", file);
    }

    return file;
}


fmi3Status recordVariables(FILE *outputFile, FMU *S, fmi3Instance s, fmi3Float64 time) {
    const fmi3ValueReference valueReferences[1] = { vr_counter };
    fmi3Int32 values[1] = { 0 };
    fmi3Status status = S->fmi3GetInt32(s, valueReferences, 1, values, 1);
    fprintf(outputFile, "%g,%d\n", time, values[0]);
    return status;
}
