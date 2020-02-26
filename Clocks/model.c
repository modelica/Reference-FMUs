#include "config.h"
#include "model.h"

#include <math.h>
#include <stdlib.h>
#include "fmi3Functions.h"


// Global variables for this model

/*

InClock_1: input, periodic	=> Clock Name c1 
InClock_2: input, aperiodic	=> Clock Name c2
InClock_3: input, aperiodic	=> Clock Name c3 //Depends on output OutClock_1
OutClock_1: output, aperiodic	=> Clock Name c4
OutClock_2: output, aperiodic	=> Clock Name c5

      time     0 1 2 3 4 5 6 7 8 9
c1 InClock_1   +       +       +  
c2 InClock_2   + +             + +
c3 InClock_3                   +  
c4 OutClock_1                  +  
c5 OutClock_2  +       +       +
      time     0 1 2 3 4 5 6 7 8 9

c_1 = t % 4 == 0
c_2 = t % 8 == 0 || (t - 1) % 8 == 0
c_3 = c_4
c_4 = count total in_clock ticks and ticks if equals 5
c_5 = t % 4 == 0


*/

void setStartValues(ModelInstance *comp) {
    M(c1)         = 0;
    M(c2)         = 0;
    M(c3)         = 0;
	M(c4)		  = 0;
	M(c5)         = 0;
	M(c1Ticks)    = 0;
	M(c2Ticks) = 0;    
	M(c3Ticks) = 0;
    M(totalInTicks) = 0;
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int* value, size_t* index) {
	switch (vr) {
	case vr_boost_c2:
		M(boost_c2) = *value;
		return OK;
	case vr_boost_c3:
		M(boost_c3) = *value;
		return OK;
	default:
		return Error;
	}
}
Status getInt32(ModelInstance* comp, ValueReference vr, int* value, size_t* index) {
	switch (vr) {
	case vr_c1Ticks:
		*value = M(c1Ticks);
		return OK;
	case vr_c2Ticks:
		*value = M(c2Ticks);
		return OK;
	case vr_c3Ticks:
		*value = M(c3Ticks);
		return OK;
	case vr_totalInTicks:
		*value = M(totalInTicks);
		return OK;
	default:
		return Error;
	}
}

void eventUpdate(ModelInstance* comp) {
	// Just to satisfy the framework. Never used here
}
void calculateValues(ModelInstance *comp) {
	// Just to satisfy the framework. Never used here
}
void mp1_run(ModelInstance* comp, fmi3Float64 time) {

	printf("mp1_run: time=%d, comp->time=%f\n", time, comp->time);

	// TODO: call lockPreemption()
	// update the counters
	M(c1Ticks)++;
	M(totalInTicks)++;

	// set output clocks
	if (M(c4) == fmi3False) M(c4) = M(totalInTicks) == 5;   // This triggers clock3
	if (M(c5) == fmi3False) M(c5) = time % 4 == 0;

	printf("mp1_run: Total count input clock ticks=%d\n", M(totalInTicks));

	comp->clocksTicked = M(c4) || M(c5);
	// TODO: call unlockPreemption()
	if (comp->clocksTicked) {
		
		fmi3IntermediateUpdateInfo updateInfo = { 0 };

		updateInfo.intermediateUpdateTime         = time;
		updateInfo.eventOccurred                  = fmi3False;
		updateInfo.clocksTicked                   = fmi3True;
		updateInfo.intermediateVariableSetAllowed = fmi3False;
		updateInfo.intermediateVariableGetAllowed = fmi3False;
		updateInfo.intermediateStepFinished       = fmi3False;
		updateInfo.canReturnEarly                 = fmi3False;
		
		comp->intermediateUpdate(comp, &updateInfo);
		
		comp->clocksTicked = fmi3False;
	}
}

void mp2_run(ModelInstance *comp, fmi3Float64 time) {
    
   
	printf("mp2_run: time=%d, comp->time=%f\n", time, comp->time);

    
    // TODO: lockPreemption()

    M(c2Ticks)++;
    M(totalInTicks)++;
    
    // TODO: unlockPreemption()
	printf("mp2_run: Total count input clock ticks=%d\n", M(totalInTicks));
}

// This is the dependent part of the model
// it is triggered as soon as output clock c4 is activated
void mp3_run(ModelInstance* comp, fmi3Float64 time) {

	
	printf("mp3_run: time=%d, comp->time=%f\n", time, comp->time);

	// TODO: lockPreemption()

	M(c3Ticks)++;
	M(totalInTicks)++;

	// TODO: unlockPreemption()
	printf("mp3_run: Total=%d\n", M(totalInTicks));
	
}

// This function is called by the master for every model partition that is supposed to run
// The model partition is identified by its clockReference
fmi3Status  ActivateModelPartition(fmi3Instance instance, fmi3ValueReference clockReference, fmi3Float64 activationTime) {
	ModelInstance* comp = instance;
	
	// IZA: Why add it to comp? Should be added as argument to mp<n>_run!
	comp->time = activationTime;
	printf("ActivateModelPartition: comp->time=%f clockReference=%d\n", comp->time, clockReference);
	switch (clockReference) {
	case vr_c1:
		mp1_run(comp, activationTime);
		break;
	case vr_c2:
		mp2_run(comp, activationTime);
		break;
	case vr_c3:
		mp3_run(comp, activationTime);
		break;
	default:
		return fmi3Error;
	}
	return fmi3OK;
}

// getClock retrieves the curren status of one specific clock.
// the state of this clock is set to inactive immediately
Status getClock(ModelInstance* comp, ValueReference vr, int* value) {

	switch (vr) {
	case vr_c1:
		*value = M(c1);
		M(c1) = fmi3False;
		return OK;
	case vr_c2:
		*value = M(c2);
		M(c2) = fmi3False;
		return OK;
	case vr_c3:
		*value = M(c3);
		M(c3) = fmi3False;
		return OK;
	case vr_c4:
		*value = M(c4);
		M(c4) = fmi3False;
		return OK;
	case vr_c5:
		*value = M(c5);
		M(c5) = fmi3False;
		return OK;
	default:
		return Error;
	}
}

// not used? To be deleted? getInt32 seems to be used instead
Status getOutput(ModelInstance* comp, ValueReference vr, int* value) {

	switch (vr) {
	case vr_c1Ticks:
		*value = M(c1Ticks);
		return fmi3OK;
	case vr_c2Ticks:
		*value = M(c2Ticks);
		return fmi3OK;
	case vr_c3Ticks:
		*value = M(c3Ticks);
		return fmi3OK;
	case vr_totalInTicks:
		*value = M(totalInTicks);
		return fmi3OK;
	default:
		return Error;
	}

}
