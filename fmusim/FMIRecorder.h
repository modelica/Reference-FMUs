#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>

typedef struct {

    double time;
    double* float64Values;

} Row;

typedef struct FMIRecorder {

    FMIInstance* instance;
    size_t nVariables;
    const FMIModelVariable** variables;
    FILE* file;
    size_t nValues;
    char* values;
    size_t* sizes;

    // data
    size_t nFloat64Variables;
    size_t nFloat64Values;
    FMIModelVariable** float64Variables;
    size_t* float64Sizes;
    FMIValueReference* float64ValueReferences;

    size_t nRows;
    Row** rows;

} FMIRecorder;

FMIRecorder* FMICreateRecorder(FMIInstance* instance, size_t nVariables, const FMIModelVariable** variables, const char* file);

void FMIFreeRecorder(FMIRecorder* result);

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* result);

FMIStatus FMIRecorderWriteCSV(FMIRecorder* recorder, FILE* file);
