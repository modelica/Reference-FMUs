#include "config.h"
#include "model.h"

#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h>
#include "fmi3Functions.h"


// Global variables for this model
 HGLOBAL globalLockVar;

/*

InClock_1: input, periodic	   // Ticks set by the master
InClock_2: input, aperiodic    // Ticks set by the master
InClock_3: input, aperiodic    // Depends on output OutClock_1
OutClock_1: output, aperiodic  // Calculated in model_part 1
OutClock_2: output, aperiodic  // Calculated in model_part 1

   time     0 1 2 3 4 5 6 7 8 9
InClock_1   + + + + + + + + + +    
InClock_2   + +             + +
InClock_3    
OutClock_1    
OutClock_2  
   time     0 1 2 3 4 5 6 7 8 9

InClock_1  = t % 4 == 0
InClock_2  = t % 8 == 0 || (t - 1) % 8 == 0
InClock_3  = triggered when OutClock_1 is triggered
OutClock_1 = count total in_clock ticks and ticks if equals 5
OutClock_2 = t % 4 == 0
*/

/*
 * SetStartValues()
 * Initialization of the memory area (=M) of the FMU
 */
void setStartValues(ModelInstance *comp) {
    M(data_InClock_1)           = 0;
    M(data_InClock_2)           = 0;
    M(data_InClock_3)           = 0;
	M(data_OutClock_1)		    = 0;
	M(data_OutClock_2)          = 0;
	M(data_InClock_1_Ticks)     = 0;
	M(data_InClock_2_Ticks)     = 0;
	M(data_InClock_3_Ticks)     = 0;
    M(data_total_InClock_Ticks) = 0;
	M(data_boost_InClock_2)     = 0;
	M(data_boost_InClock_3)     = 0;
}

/*
 * setInt32()
 * Implementation of the fmi3SetInteger32 functionality
 * Sets the value of a variable identified by its value reference
 */
Status setInt32(ModelInstance* comp, ValueReference vr, const int* value, size_t* index) {
	switch (vr) {
	case vr_boost_InClock_2:
		M(data_boost_InClock_2) = *value;
		return OK;
	case vr_boost_InClock_3:
		M(data_boost_InClock_3) = *value;
		return OK;
	default:
		return Error;
	}
}

/*
 * getInt32()
 * Implementation of the fmi3GetInteger32 functionality
 * return the value of one variable identified by its value reference
 */
Status getInt32(ModelInstance* comp, ValueReference vr, int* value, size_t* index) {
	switch (vr) {
	case vr_InClock_1_Ticks:
		*value = M(data_InClock_1_Ticks);
		return OK;
	case vr_InClock_2_Ticks:
		*value = M(data_InClock_2_Ticks);
		return OK;
	case vr_InClock_3_Ticks:
		*value = M(data_InClock_3_Ticks);
		return OK;
	case vr_total_InClock_Ticks:
		*value = M(data_total_InClock_Ticks);
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

/*
* Runnable function for model partition 1
* triggered by the master
*/
void mp1_run(ModelInstance* comp, double time) {
	int  threadPrio = GetThreadPriority(GetCurrentThread());
	logEvent(comp, "mp1_run: time=%g, comp->time=%g, prio=%d Processor= %d", time, comp->time, threadPrio, GetCurrentProcessorNumber());

	// TODO: call lockPreemption()
	if (GlobalLock(globalLockVar) == NULL) { logEvent(comp, "mp1_run: Retrieving lock failed: %ld", GetLastError()); return; }

	// update the counters
	M(data_InClock_1_Ticks)++;
	M(data_total_InClock_Ticks)++;

	// set output clocks
	if ((M(data_OutClock_1) == fmi3ClockInactive) && (M(data_total_InClock_Ticks) == 5)) {   
		M(data_OutClock_1) = fmi3ClockActive; // Set the trigger for outClock_1  => the master will then run the task for model partition 3
	}
	logEvent(comp, "mp1_run: Total count input clock ticks=%d", M(data_total_InClock_Ticks));

	comp->clocksTicked = M(data_OutClock_1);

	// TODO: call unlockPreemption()
	GlobalUnlock(globalLockVar);

	if (comp->clocksTicked) {
		fmi3IntermediateUpdateInfo updateInfo = { 0 };

		updateInfo.intermediateUpdateTime         = time;
		updateInfo.eventOccurred                  = fmi3False;
		updateInfo.clocksTicked                   = fmi3True;   // Only member, we are interested in
		updateInfo.intermediateVariableSetAllowed = fmi3False;
		updateInfo.intermediateVariableGetAllowed = fmi3False;
		updateInfo.intermediateStepFinished       = fmi3False;
		updateInfo.canReturnEarly                 = fmi3False;
		
		comp->intermediateUpdate(comp, &updateInfo);
		comp->clocksTicked = fmi3False;
	}
}

/*
* Runnable function for model partition 2
* triggered by the master
*/
void mp2_run(ModelInstance *comp, double time) {
	int  threadPrio = GetThreadPriority(GetCurrentThread());
	logEvent(comp, "mp2_run: time=%g, comp->time=%g, prio=%d Processor= %d", time, comp->time, threadPrio, GetCurrentProcessorNumber());

	// TODO: lockPreemption()
	if (GlobalLock(globalLockVar) == NULL) { logEvent(comp, "mp2_run: Retrieving lock failed: %ld", GetLastError()); return; }

    M(data_InClock_2_Ticks)++;
    M(data_total_InClock_Ticks)++;
	if ((M(data_OutClock_2) == fmi3ClockInactive) && (fmod(time, 4) == 0))
		M(data_OutClock_2) = fmi3ClockActive;
    
	// TODO: unlockPreemption()
	GlobalUnlock(globalLockVar);

	logEvent(comp, "mp2_run: Total count input clock ticks=%d", M(data_total_InClock_Ticks));
}

/*
 * Runnable function for model partition 3
 * This is the dependent part of the model
 * it is triggered when OutClock_1 is activated

 * Calculating some sum in order to waste CPU cycles and 
 * thus show that it can be interrupted by higher prio tasks
 */
void mp3_run(ModelInstance* comp, double time) {
	int  threadPrio = GetThreadPriority(GetCurrentThread());
	logEvent(comp, "mp3_run: time=%g, comp->time=%g, prio=%d Processor= %d", time, comp->time, threadPrio, GetCurrentProcessorNumber());

	// TODO: lockPreemption()
	if (GlobalLock(globalLockVar) == NULL) { logEvent(comp, "mp3_run: Retrieving lock failed: %ld", GetLastError()); return; }

	M(data_InClock_3_Ticks)++;
	M(data_total_InClock_Ticks)++;

	// TODO: unlockPreemption()
	GlobalUnlock(globalLockVar);

	// This partition is supposed to consume a bit of time on a low prio ...
	unsigned long sum = 0;
	for (int loop = 1; loop < 2000000000; loop++) {
		sum += loop;
	}
	// ... end of burning CPU cycles

	logEvent(comp, "mp3_run: finished calculation: sum=%lu Total=%d", sum, M(data_total_InClock_Ticks));
}

/*
* activateModelPartition(modelinstance, clockreference, time)
* This function is called by the master for every model partition that is supposed to run
* Everyy call of this function will be done within a separate thread, which is created by the master
* The model partition is identified by its clockReference
* returns Ok on success
*         Error otherwise
*/
Status activateModelPartition(ModelInstance* comp, ValueReference clockReference, double activationTime) {
	
	logEvent(comp, "ActivateModelPartition: comp->time=%f clockReference=%d", activationTime, clockReference);
	
	switch (clockReference) {
	case vr_InClock_1:
		mp1_run(comp, activationTime);
		break;
	case vr_InClock_2:
		mp2_run(comp, activationTime);
		break;
	case vr_InClock_3:
		mp3_run(comp, activationTime);
		break;
	default:
		return Error;
	}
	return OK;
}

/*
 * getClock()
 * retrieve the current status of one specific clock
 * After retrieving, the state is set to inactive immediately.
 * That way, for two succeeding calls of this function for the same clock, only the first one can show fmi3ClockActive
 */
Status getClock(ModelInstance* comp, ValueReference vr, int* value) {

	switch (vr) {
	case vr_InClock_1:
		*value = M(data_InClock_1);
		M(data_InClock_1) = fmi3ClockInactive;
		return OK;
	case vr_InClock_2:
		*value = M(data_InClock_2);
		M(data_InClock_2) = fmi3ClockInactive;
		return OK;
	case vr_InClock_3:
		*value = M(data_InClock_3);
		M(data_InClock_3) = fmi3ClockInactive;
		return OK;
	case vr_OutClock_1:
		*value = M(data_OutClock_1);
		M(data_OutClock_1) = fmi3ClockInactive;
		return OK;
	case vr_OutClock_2:
		*value = M(data_OutClock_2);
		M(data_OutClock_2) = fmi3ClockInactive;
		return OK;
	default:
		return Error;
	}
}
