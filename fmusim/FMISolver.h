#pragma once

#include "FMIModelDescription.h"
#include "fmusim_input.h"


typedef struct SolverImpl Solver;

typedef Solver* (*SolverCreate)(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double tolerance, double startTime);

typedef void (*SolverFree)(Solver* solver);

typedef FMIStatus (*SolverStep)(void* solver, double nextTime, double* timeReached, bool* stateEvent);

typedef FMIStatus (*SolverReset)(void* solver, double time);
