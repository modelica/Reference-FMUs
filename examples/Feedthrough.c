#include <stdio.h>
#include "FMU.h"
#include "config.h"


FILE *openOutputFile(const char *filename) {

    FILE *outputFile = fopen(filename, "w");

    if (!outputFile) {
        NULL;
    }

    fputs("time,continuous_real_in,continuous_real_out", outputFile);

    return outputFile;
}


fmi3Status recordVariables(FILE *outputFile, FMU *S, fmi3Instance s, fmi3Float64 time) {
    const fmi3ValueReference valueReferences[2] = { vr_continuous_real_in, vr_continuous_real_out };
    fmi3Float64 values[2] = { 0 };
    fmi3Status status = S->fmi3GetFloat64(s, valueReferences, 2, values, 2);
    fprintf(outputFile, "%g,%g,%g\n", time, values[0], values[1]);
    return status;
}
