#pragma once

#include "FMISolver.h"


Solver* FMICVodeCreate(FMIInstance* S, const FMIModelDescription* modelDescription, const FMUStaticInput* input, double startTime);

void FMICVodeFree(Solver* solver);

FMIStatus FMICVodeStep(Solver* solver, double nextTime, double* timeReached, bool* stateEvent);

FMIStatus FMICVodeReset(Solver* solver, double time);
