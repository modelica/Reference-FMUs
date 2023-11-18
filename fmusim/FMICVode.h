#pragma once

#include "FMISolver.h"


Solver* FMICVodeCreate(const FMISolverParameters* solverFunctions);

void FMICVodeFree(Solver* solver);

FMISolverStatus FMICVodeStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMISolverStatus FMICVodeReset(Solver* solver, double time);
