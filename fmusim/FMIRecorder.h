#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>

#define N_VARIABLE_TYPES (FMIClockType + 1)

typedef struct {

    double time;
    void* values[N_VARIABLE_TYPES];
    size_t* sizes;

} Row;

typedef struct {

    size_t nVariables;
    size_t nValues; ///> total number of values
    FMIModelVariable** variables;
    size_t* sizes;  ///> values per variable 
    FMIValueReference* valueReferences;

} VariableInfo;

typedef struct FMIRecorder {

    FMIInstance* instance;
    size_t nVariables;
    const FMIModelVariable** variables;
    FMIValueReference* valueReferences;
    VariableInfo* variableInfos[N_VARIABLE_TYPES];

    size_t nRows;
    Row** rows;

} FMIRecorder;

FMIRecorder* FMICreateRecorder(FMIInstance* instance, size_t nVariables, const FMIModelVariable** variables, const char* file);

void FMIFreeRecorder(FMIRecorder* recorder);

FMIStatus FMIRecorderUpdateSizes(FMIRecorder* recorder);

FMIStatus FMISample(FMIInstance* instance, double time, FMIRecorder* recorder);

FMIStatus FMIRecorderWriteCSV(FMIRecorder* recorder, FILE* file);
