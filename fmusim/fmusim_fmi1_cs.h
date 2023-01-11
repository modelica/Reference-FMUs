#pragma once

#include "FMI1.h"
#include "FMIModelDescription.h"
#include "FMIRecorder.h"
#include "fmusim_input.h"
#include "fmusim.h"


FMIStatus simulateFMI1CS(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription,
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput* input,
    const FMISimulationSettings* settings);
