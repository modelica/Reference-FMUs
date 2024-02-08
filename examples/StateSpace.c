#define NO_INPUTS

#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,y\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, double time, FILE *outputFile) {

    const fmi3ValueReference vr_r_ = vr_r;

    fmi3UInt64 r;

    FMIStatus status;

    status = FMI3GetUInt64(S, &vr_r_, 1, &r, 1);

    if (status > FMIWarning) return status;

    const fmi3ValueReference vr_y_ = vr_y;

    fmi3Float64 y[R_MAX] = { 0 };

    status = FMI3GetFloat64(S, &vr_y_, 1, y, r);

    if (status > FMIWarning) return status;

    fprintf(outputFile, "%g,", time);

    for (size_t i = 0; i < r; i++) {
        fprintf(outputFile, i == 0 ? "%g" : " %g", y[i]);
    }

    fputc('\n', outputFile);

    return status;
}
