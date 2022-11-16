#pragma once

#include "FMI2.h"
#include "FMIModelDescription.h"
#include "fmusim_input.h"


typedef struct SolverImpl Solver;

typedef Solver* (*SolverCreate)(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime);

typedef void (*SolverFree)(Solver* solver);

typedef void (*SolverStep)(void* solver, double nextTime, double* timeReached, bool* stateEvent);

typedef void (*SolverReset)(void* solver, double time);
