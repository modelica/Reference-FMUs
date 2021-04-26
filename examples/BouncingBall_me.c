#define FIXED_STEP 1e-2
#define STOP_TIME 3
#define OUTPUT_FILE_HEADER "time,h,v\n"

#include "simulate_me.h"

fmi3Status recordVariables(FILE *outputFile, FMU *S, fmi3Instance s, fmi3Float64 time) {
    const fmi3ValueReference valueReferences[2] = { vr_h, vr_v };
    fmi3Float64 values[2] = { 0 };
    fmi3Status status = S->fmi3GetFloat64(s, valueReferences, 2, values, 2);
    fprintf(outputFile, "%g,%g,%g\n", time, values[0], values[1]);
    return status;
}
