#pragma once

#include "FMIModelDescription.h"
#include "FMISolver.h"
#include "FMIRecorder.h"
#include "FMIStaticInput.h"


typedef struct {

    // Common
    FMIInterfaceType interfaceType;
    size_t nStartValues;
    const FMIModelVariable** startVariables;
    const char** startValues;
    double tolerance;
    double startTime;
    double stopTime;
    double outputInterval;
    const char* initialFMUStateFile;
    const char* finalFMUStateFile;
    bool visible;
    bool loggingOn;

    // Co-Simulation
    bool earlyReturnAllowed;
    bool eventModeUsed;
    bool recordIntermediateValues;

    // Model Exchange
    SolverCreate solverCreate;
    SolverFree solverFree;
    SolverStep solverStep;
    SolverReset solverReset;

} FMISimulationSettings;

FMIStatus FMIApplyStartValues(FMIInstance* S, const FMISimulationSettings* settings);

FMIStatus FMISimulate(
    FMIInstance* S,
    const FMIModelDescription* modelDescription,
    const char* unzipdir,
    FMIRecorder* recorder,
    const FMIStaticInput* input,
    const FMISimulationSettings* settings);
