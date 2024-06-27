#pragma once

#include "FMISimulation.h"


FMIStatus FMI1MESimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    FMIRecorder* result,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
