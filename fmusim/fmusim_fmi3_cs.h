#pragma once

#include "FMI3.h"
#include "FMIModelDescription.h"
#include "FMISimulationResult.h"
#include "fmusim_input.h"
#include "fmusim.h"


FMIStatus simulateFMI3CS(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourcePath,
    FMISimulationResult* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
