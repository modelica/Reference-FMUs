#pragma once

#include "FMI3.h"
#include "FMIModelDescription.h"
#include "FMISimulationResult.h"


FMIStatus simulateFMI3CS(FMIInstance* S, const char* instantiationToken, const char* resourcePath,
    FMISimulationResult* result,
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[],
    double startTime,
    double stepSize,
    double stopTime,
    bool earlyReturnAllowed);