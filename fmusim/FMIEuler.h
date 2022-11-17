#pragma once

#include "FMISolver.h"


Solver* FMIEulerCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime);

void FMIEulerFree(Solver* solver);

FMIStatus FMIEulerStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMIStatus FMIEulerReset(Solver* solver, double time);
