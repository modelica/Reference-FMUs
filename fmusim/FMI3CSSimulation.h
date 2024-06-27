#pragma once

#include "FMISimulation.h"


FMIStatus FMI3CSSimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourcePath,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
