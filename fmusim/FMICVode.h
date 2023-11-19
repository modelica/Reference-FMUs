#pragma once

#include "FMISolver.h"


FMISolver* FMICVodeCreate(const FMISolverParameters* solverFunctions);

void FMICVodeFree(FMISolver* solver);

FMISolverStatus FMICVodeStep(FMISolver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMISolverStatus FMICVodeReset(FMISolver* solver, double time);
