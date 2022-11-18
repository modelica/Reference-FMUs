#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>


typedef struct {

    size_t nVariables;
    const FMIModelVariable** variables;
    FILE* file;

} FMISimulationResult;

FMISimulationResult* FMICreateSimulationResult(size_t nVariables, const FMIModelVariable* variables[], const char* file);

void FMIFreeSimulationResult(FMISimulationResult* result);

FMIStatus FMISample(FMIInstance* instance, double time, FMISimulationResult* result);
