#pragma once

#include "FMIRecorder.h"


FMIRecorder* FMIDemoRecorderCreate(FMIInstance* instance);

void FMIDemoRecorderFree(FMIRecorder* recorder);

FMIStatus FMIDemoRecorderSample(FMIRecorder* recorder, double time);

const double* FMIDemoRecorderValues(FMIRecorder* recorder, size_t *rows);
