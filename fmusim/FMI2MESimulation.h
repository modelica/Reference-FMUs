#pragma once

#include "FMISimulation.h"


FMIStatus FMI2MESimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
