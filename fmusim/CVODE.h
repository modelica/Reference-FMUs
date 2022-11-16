#pragma once

#include "FMISolver.h"


Solver* CVODECreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime);

void CVODEFree(Solver* solver);

FMIStatus CVODEStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMIStatus CVODEReset(Solver* solver, double time);
