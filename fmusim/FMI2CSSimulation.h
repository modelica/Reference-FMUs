#pragma once

#include "FMISimulation.h"


FMIStatus FMI2CSSimulate(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
