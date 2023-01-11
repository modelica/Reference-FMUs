#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>


typedef struct {

    FMIInstance* instance;
    size_t nVariables;
    const FMIModelVariable** variables;
    FILE* file;
    size_t nValues;
    char* values;
    size_t* sizes;

} FMIRecorder;

FMIRecorder* FMICreateRecorder(size_t nVariables, const FMIModelVariable* variables[], const char* file);

void FMIFreeRecorder(FMIRecorder* result);

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* result);
