#include "util.h"


FILE *createOutputFile(const char *filename) {

    FILE *file = fopen(filename, "w");

    if (file) {
        fputs("time,inClock1Ticks,inClock2Ticks,inClock3Ticks,totalInClockTicks\n", file);
    }

    return file;
}

FMIStatus recordVariables(FMIInstance *S, double time, FILE *outputFile) {

    const fmi3ValueReference valueReferences[4] = { vr_inClock1Ticks, vr_inClock2Ticks, vr_inClock3Ticks, vr_totalInClockTicks };

    fmi3Int32 values[4] = { 0 };

    const FMIStatus getStatus = FMI3GetInt32(S, valueReferences, 4, values, 4);

    fprintf(outputFile, "%g,%d,%d,%d,%d\n", time, values[0], values[1], values[2], values[3]);

    return getStatus;
}
