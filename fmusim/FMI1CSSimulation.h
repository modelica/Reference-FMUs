#pragma once

#include "FMISimulation.h"


FMIStatus simulateFMI1CS(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
