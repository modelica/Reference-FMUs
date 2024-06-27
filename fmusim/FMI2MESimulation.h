#pragma once

#include "FMISimulation.h"


FMIStatus simulateFMI2ME(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
