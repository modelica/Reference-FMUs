#pragma once

#include "FMISimulation.h"


FMIStatus FMI3MESimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMIRecorder* result,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
