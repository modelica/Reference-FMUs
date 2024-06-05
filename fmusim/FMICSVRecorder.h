#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>


typedef struct FMIRecorderImpl FMIRecorder;

FMIRecorder* FMICSVRecorderCreate(FMIInstance* instance, size_t nVariables, FMIModelVariable** variables, const char* file);

void FMICSVRecorderFree(FMIRecorder* recorder);

FMIStatus FMICSVRecorderSample(FMIRecorder* recorder, double time);
