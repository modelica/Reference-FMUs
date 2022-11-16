#pragma once

#include "FMISolver.h"


Solver* ForwardEulerCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime);

void ForwardEulerFree(Solver* solver);

void ForwardEulerStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

void ForwardEulerReset(Solver* solver, double time);
