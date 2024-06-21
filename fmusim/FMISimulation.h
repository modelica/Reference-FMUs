#pragma once

#include "FMIModelDescription.h"
#include "FMISolver.h"
#include "FMIRecorder.h"
#include "fmusim_input.h"


typedef struct {

    // Common
    bool visible;
    bool loggingOn;
    size_t nStartValues;
    FMIModelVariable** startVariables;
    char** startValues;
    double tolerance;
    double startTime;
    double stopTime;
    double outputInterval;
    const char* initialFMUStateFile;
    const char* finalFMUStateFile;

    // Input
    FMUStaticInput* input;

    // Recorder
    FMIRecorder* recorder;
    FMIRecorderSample sample;

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
