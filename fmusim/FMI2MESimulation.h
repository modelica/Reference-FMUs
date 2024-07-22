#pragma once

#include "FMISimulation.h"


FMIStatus FMI2MESimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* recorder,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
