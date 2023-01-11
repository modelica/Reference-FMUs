#pragma once

#include "FMI3.h"
#include "FMIModelDescription.h"
#include "FMIRecorder.h"
#include "fmusim_input.h"
#include "fmusim.h"


FMIStatus simulateFMI3ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
