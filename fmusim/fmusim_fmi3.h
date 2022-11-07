#pragma once

#include "FMI3.h"
#include "FMIModelDescription.h"


FMIStatus applyStartValuesFMI3(
    FMIInstance* S, 
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[]);
