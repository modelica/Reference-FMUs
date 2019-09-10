#include "config.h"
#include "model.h"
#include <math.h>
#include <stdlib.h>

/*

c_1: input, periodic
c_2: input, aperiodic
c_3: output, periodic
c_4: output, aperiodic

c_1 +   +   +
c_2 ++      ++
c_3 - - - - -
c_4 --  --  --
t   0123456789

c_1 = t % 4 == 0
c_2 = t % 8 == 0 || (t - 1) % 8 == 0
c_3 = t % 2 == 0
c_4 = t % 4 == 0 || (t - 1) % 4 == 0

*/

void setStartValues(ModelInstance *comp) {
    M(c1)         = 0;
    M(c2)         = 0;
    M(c3)         = 0;
    M(c4)         = 0;
	M(c1Ticks)    = 0;
	M(c2Ticks)    = 0;
	M(totalTicks) = 0;
}

void calculateValues(ModelInstance *comp) {
    // TODO
}

void eventUpdate(ModelInstance *comp) {
    
    int time = comp->time;
    
    if (time != comp->time) {
        logError(comp, "Time must be a multiple of 1.");
    }

	// update event time
	double c3      = 2 * (time / 2 + 1);
	double c4_even = 4 * (time / 4 + 1);
	double c4_odd  = 4 * (time / 4 + 1) + 1;

	int nextEventTime = fmin(c3, c4_even);
	nextEventTime = fmin(nextEventTime, c4_odd);

	// TODO: lockPreemption()

	// set output clocks
	M(c3) = time % 2 == 0;
	M(c4) = time % 4 == 0 || (time - 1) % 4 == 0;

	// update the counters
	if (M(c1)) {
		M(c1Ticks)++;
		M(totalTicks)++;
	}

	if (M(c2)) {
		M(c2Ticks)++;
		M(totalTicks)++;
	}

	// TODO: unlockPreemption()

	comp->valuesOfContinuousStatesChanged   = false;
	comp->nominalsOfContinuousStatesChanged = false;
	comp->terminateSimulation               = false;
	comp->nextEventTime                     = nextEventTime;
	comp->nextEventTimeDefined              = true;
	comp->clocksTicked                      = M(c3) || M(c4);
}

Status activateClock(ModelInstance* comp, ValueReference vr) {
    return OK;
}

Status getClock(ModelInstance* comp, ValueReference vr, int* value) {

	switch (vr) {
	case vr_c1:
		*value = M(c1);
		return OK;
	case vr_c2:
		*value = M(c2);
		return OK;
	case vr_c3:
		*value = M(c3);
		return OK;
    case vr_c4:
        *value = M(c4);
        return OK;
    case vr_c1Ticks:
        *value = M(c1Ticks);
        return OK;
    case vr_c2Ticks:
        *value = M(c2Ticks);
        return OK;
    case vr_totalTicks:
        *value = M(totalTicks);
        return OK;
	default:
		return Error;
	}

}
