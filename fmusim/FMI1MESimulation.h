#pragma once

#include "FMISimulation.h"


FMIStatus FMI1MESimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
