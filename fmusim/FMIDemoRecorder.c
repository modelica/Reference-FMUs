#include <inttypes.h>
#include <stdlib.h>

#include "FMI1.h"
#include "FMI2.h"
#include "FMI3.h"
#include "FMIUtil.h"

#include "FMIDemoRecorder.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


struct FMIRecorderImpl {

    FMIInstance* instance;
    size_t row;
    double values[1000];

} FMIDemoRecorderImpl;

FMIRecorder* FMIDemoRecorderCreate(FMIInstance* instance) {

    FMIStatus status = FMIOK;

    FMIRecorder* recorder = NULL;
    
    CALL(FMICalloc((void**)&recorder, 1, sizeof(FMIDemoRecorderImpl)));

    recorder->instance = instance;

TERMINATE:

    if (status != FMIOK) {
        FMIFree((void**)&recorder);
    }

    return recorder;
}

void FMIDemoRecorderFree(FMIRecorder* recorder) {
    if (recorder) {
        free(recorder);
    }
}

FMIStatus FMIDemoRecorderSample(FMIRecorder* recorder, double time) {

    FMIStatus status = FMIOK;

    if (!recorder) {
        goto TERMINATE;
    }

    FMIInstance *instance = recorder->instance;

    fmi3ValueReference vr = 1; // h
    fmi3Float64 value;

    CALL(FMI3GetFloat64(instance, &vr, 1, &value, 1));

    if (recorder->row < 500) {
        recorder->values[2 * recorder->row] = time;
        recorder->values[2 * recorder->row + 1] = value;
    }

    recorder->row++;

TERMINATE:
    return status;
}

const double* FMIDemoRecorderValues(FMIRecorder* recorder, size_t *rows) {
    *rows = recorder->row;
    return recorder->values;
}
