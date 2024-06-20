#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>

typedef struct {

    double time;
    double* float64Values;

} Row;

struct FMIRecorderImpl {

    FMIInstance* instance;
    size_t nVariables;
    const FMIModelVariable** variables;
    FILE* file;
    size_t nValues;
    char* values;
    size_t* sizes;
    //size_t dataRowSize;
    //size_t allocatedDataSize;
    //size_t nDataRows;
    //char* data;

    size_t nFloat64Variables;
    size_t nFloat64Values;
    FMIModelVariable** float64Variables;
    size_t* float64Sizes;
    FMIValueReference* float64ValueReferences;

    size_t nRows;
    Row** rows;

} FMIRecorderImpl_CSV;

typedef struct FMIRecorderImpl FMIRecorder;

FMIRecorder* FMICSVRecorderCreate(FMIInstance* instance, size_t nVariables, FMIModelVariable** variables, const char* file);

void FMICSVRecorderFree(FMIRecorder* recorder);

FMIStatus FMICSVRecorderSample(FMIRecorder* recorder, double time);

void FMICSVRecorderDump(FMIRecorder* recorder);
