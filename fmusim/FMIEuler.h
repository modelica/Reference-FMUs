#pragma once

#include "FMISolver.h"


FMISolver* FMIEulerCreate(const FMISolverParameters* solverFunctions);

void FMIEulerFree(FMISolver* solver);

FMISolverStatus FMIEulerStep(FMISolver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMISolverStatus FMIEulerReset(FMISolver* solver, double time);
