#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>


typedef struct {

    size_t nVariables;
    const FMIModelVariable** variables;
    FILE* file;

} FMIRecorder;

FMIRecorder* FMICreateRecorder(size_t nVariables, const FMIModelVariable* variables[], const char* file);

void FMIFreeRecorder(FMIRecorder* result);

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* result);
