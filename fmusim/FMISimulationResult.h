#ifndef FMI_SIMULATION_RESULT_H
#define FMI_SIMULATION_RESULT_H

/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

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

#endif  // FMI_SIMULATION_RESULT_H