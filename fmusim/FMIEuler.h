#pragma once

#include "FMISolver.h"


Solver* FMIEulerCreate(const FMISolverParameters* solverFunctions);

void FMIEulerFree(Solver* solver);

FMISolverStatus FMIEulerStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMISolverStatus FMIEulerReset(Solver* solver, double time);
