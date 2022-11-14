#pragma once

#include "FMI2.h"
#include "FMIModelDescription.h"


FMIStatus applyStartValuesFMI2(
    FMIInstance* S, 
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[]);
