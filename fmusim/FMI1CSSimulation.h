#pragma once

#include "FMISimulation.h"


FMIStatus FMI1CSSimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* recorder,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
