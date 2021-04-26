#define FIXED_STEP 1e-2
#define STOP_TIME 3
#define OUTPUT_FILE_HEADER "time,counter\n"

#include "simulate_me.h"

fmi3Status recordVariables(FILE *outputFile, FMU *S, fmi3Instance s, fmi3Float64 time) {
    const fmi3ValueReference valueReferences[1] = { vr_counter };
    fmi3Int32 values[1] = { 0 };
    fmi3Status status = S->fmi3GetInt32(s, valueReferences, 1, values, 1);
    fprintf(outputFile, "%g,%d\n", time, values[0]);
    return status;
}
