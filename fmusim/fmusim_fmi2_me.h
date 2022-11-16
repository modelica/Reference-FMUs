#pragma once

#include "FMI2.h"
#include "FMIModelDescription.h"
#include "FMISimulationResult.h"
#include "fmusim_input.h"
#include "fmusim.h"


FMIStatus simulateFMI2ME(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMISimulationResult* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
