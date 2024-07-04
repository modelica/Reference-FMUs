#pragma once

#include "FMISimulation.h"


FMIStatus FMI3MESimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMIRecorder* recorder,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
