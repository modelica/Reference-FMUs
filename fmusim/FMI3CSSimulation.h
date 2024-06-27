#pragma once

#include "FMISimulation.h"


FMIStatus simulateFMI3CS(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourcePath,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
