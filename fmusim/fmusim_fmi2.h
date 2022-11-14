#pragma once

/**************************************************************
 *  Copyright (c) Modelica Association Project "FMI".         *
 *  All rights reserved.                                      *
 *  This file is part of the Reference FMUs. See LICENSE.txt  *
 *  in the project root for license information.              *
 **************************************************************/

#include "FMI2.h"
#include "FMIModelDescription.h"


FMIStatus applyStartValuesFMI2(
    FMIInstance* S, 
    size_t nStartValues,
    const FMIModelVariable* startVariables[],
    const char* startValues[]);
