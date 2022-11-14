#pragma once

#include "FMIModelDescription.h"
#include <stdio.h>


typedef struct {

    size_t nVariables;
    FMIModelVariable** variables;
    size_t nSamples;
    size_t sampleSize;
    size_t dataSize;
    char* data;

} FMISimulationResult;

FMISimulationResult* FMICreateSimulationResult(FMIModelDescription* modelDescription);

void FMIFreeSimulationResult(FMISimulationResult* result);

FMIStatus FMISample(FMIInstance* instance, double time, FMISimulationResult* result);

void FMIDumpResult(FMISimulationResult* result, FILE* file);
