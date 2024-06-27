#pragma once

#include "FMISimulation.h"


FMIStatus FMI1CSSimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
