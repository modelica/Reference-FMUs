#pragma once

#include "FMISimulation.h"


FMIStatus FMI3CSSimulate(
    //FMIInstance* S, 
    //const FMIModelDescription* modelDescription,
    const char* resourcePath,
    const FMISimulationSettings* settings);
