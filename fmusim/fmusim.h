#pragma once

#include "FMIModelDescription.h"
#include "FMISolver.h"

typedef struct {

    // Common
    size_t nStartValues;
    const FMIModelVariable** startVariables;
    const char** startValues;
    double startTime;
    double stopTime;
    double outputInterval;

    // Co-Simulation
    bool earlyReturnAllowed;

    // Model Exchange
    SolverCreate solverCreate;
    SolverFree solverFree;
    SolverStep solverStep;
    SolverReset solverReset;

} FMISimulationSettings;

FMIStatus applyStartValues(FMIInstance* S, const FMISimulationSettings* settings);
