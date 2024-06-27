#pragma once

#include "FMISimulation.h"


FMIStatus simulateFMI3ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
