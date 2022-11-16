#pragma once

#include "FMISolver.h"


Solver* ForwardEulerCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime);

void ForwardEulerFree(Solver* solver);

FMIStatus ForwardEulerStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMIStatus ForwardEulerReset(Solver* solver, double time);
