#pragma once

#include "FMISimulation.h"


FMIStatus FMI1MESimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    FMIRecorder* recorder,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
