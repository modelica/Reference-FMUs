#pragma once

#include "FMI2.h"
#include "FMIModelDescription.h"
#include "FMISimulationResult.h"


FMIStatus simulateFMI2ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime);
