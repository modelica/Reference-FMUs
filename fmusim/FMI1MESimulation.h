#pragma once

#include "FMISimulation.h"


FMIStatus simulateFMI1ME(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
