#include "config.h"
#include "model.h"
#include <math.h>
#include <stdlib.h>

/*

time        0 1 2 3 4 5 6 7 8 9
inClock1    + + + + + + + + + +   t % 4 == 0
inClock2    + +             + +   t % 8 == 0 || (t - 1) % 8 == 0
inClock3            +             countdown depends on inClock1
outClock    ? ? ? ? ? ? ? ? ? ?   totalInTicks % 5 == 0 (triggered by all inClocks)
time        0 1 2 3 4 5 6 7 8 9

*/

/**************************************
ModelPartition 1 does the following:
  - increments the clock tick counters
  - if time is 4, countdown clock inClock3's interval is set (which will trigger Model Partition 3)
  - triggers outClock, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition1(ModelInstance* comp, double time) {

    if (comp->lockPreemtion) {
        comp->lockPreemtion();
    }

    // increment the counters
    M(inClock1Ticks)++;
    M(totalInClockTicks)++;

    // set countdown and output clocks
    if ((int)time == 4) {
        M(inClock3_qualifier)= 2; // fmi3IntervalChanged
        M(inClock3_interval) = 0.0;
    }

    M(outClock) = ((M(outClock) == false) && (M(totalInClockTicks) % 5 == 0));

    if (comp->unlockPreemtion) {
        comp->unlockPreemtion();
    }

    if (M(inClock3_qualifier) == 2 || M(outClock)) {
        comp->clockUpdate(comp->componentEnvironment);
    }
}

/**************************************
ModelPartition 2 does the following:
  - increments the clock tick counters
  - gets an input value (from ModelPartition3)
  - triggers outClock, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition2(ModelInstance* comp, double time) {

    UNUSED(time);

    if (comp->lockPreemtion) {
        comp->lockPreemtion();
    }

    // increment the counters
    M(inClock2Ticks)++;
    M(totalInClockTicks)++;

    M(result2) += M(input2);  // add the output from mp3
    M(input2) = 0;    // then reset the value

    // set output clocks
    M(outClock) = ((M(outClock) == false) && (M(totalInClockTicks) % 5 == 0));

    if (comp->unlockPreemtion) {
        comp->unlockPreemtion();
    }

    if (M(outClock)) {
        comp->clockUpdate(comp->componentEnvironment);
    }
}

 /**************************************
 ModelPartition 3 does the following:
   - increments the clock tick counters
   - burns some CPU cycles by calculating some nonsense
   - sets an output variable that is supposed to find its way into ModelPartition 2
   - triggers outClock, if the number of totalInTicks is a multiple of 5
 **************************************/
 static void activateModelPartition3(ModelInstance *comp, double time) {

    if (comp->lockPreemtion) {
        comp->lockPreemtion();
    }

    // increment the counters
    M(inClock3Ticks)++;

    if (comp->unlockPreemtion) {
        comp->unlockPreemtion();
    }

    // This partition is supposed to consume a bit of time on a low prio ...
    unsigned long sum = 0;
    for (int loop = 1; loop < 1000000000; loop++) {
        sum += loop;
    }
    // ... end of burning CPU cycles
    M(output3) = 1000;   // this is suposed to find its way into mp2
    M(totalInClockTicks)++;

    // set output clocks
    M(outClock) = ((M(outClock) == false) && (M(totalInClockTicks) % 5 == 0));
    if (M(outClock)) {

        bool earlyReturnRequested;
        double earlyReturnTime;

        if (M(outClock)) {
            comp->intermediateUpdate(
                comp,   // fmu instance
                time,   // intermediateUpdateTime
                false,  // intermediateVariableSetAllowed
                false,  // intermediateVariableGetAllowed
                true,   // intermediateStepFinished
                false,  // canReturnEarly
                &earlyReturnRequested,
                &earlyReturnTime
            );
        }
    }
}

void setStartValues(ModelInstance *comp) {
    M(inClock3_interval) = 0.0;
    M(inClock3_qualifier)= 0; // fmi3IntervalNotYetKnown
    M(outClock)          = 0;
    M(inClock1Ticks)     = 0;
    M(inClock2Ticks)     = 0;
    M(inClock3Ticks)     = 0;
    M(totalInClockTicks) = 0;
    M(result2)           = 0;
    M(input2)            = 0;
    M(output3)           = 0;
}

Status calculateValues(ModelInstance *comp) {

    UNUSED(comp);

    return OK;
}

void eventUpdate(ModelInstance *comp) {
    UNUSED(comp);
}

Status activateClock(ModelInstance* comp, ValueReference vr) {

    UNUSED(comp);
    UNUSED(vr);

    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
    case vr_time:
        value[(*index)++] = comp->time;
        return OK;
    default:
        logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int* value, size_t* index) {
    switch (vr) {
    case vr_input2:
        M(input2) = value[(*index)++];
        return OK;
    default:
        logError(comp, "Set Int32 is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {

    switch (vr) {
    case vr_inClock1Ticks:
        value[(*index)++] = M(inClock1Ticks);
        return OK;
    case vr_inClock2Ticks:
        value[(*index)++] = M(inClock2Ticks);
        return OK;
    case vr_inClock3Ticks:
        value[(*index)++] = M(inClock3Ticks);
        return OK;
    case vr_totalInClockTicks:
        value[(*index)++] = M(totalInClockTicks);
        return OK;
    case vr_result2:
        value[(*index)++] = M(result2);
        return OK;
    case vr_output3:
        value[(*index)++] = M(output3);
        return OK;
    default:
        logError(comp, "Get Int32 is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status getClock(ModelInstance* comp, ValueReference vr, _Bool *value) {

    switch (vr) {
    case vr_outClock:
        *value = M(outClock);
        M(outClock) = false;
        return OK;
    default:
        logError(comp, "Get Clock is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status getInterval(ModelInstance* comp, ValueReference vr, double* interval, int* qualifier) {
    switch (vr) {
    case vr_inClock3:
        *qualifier = M(inClock3_qualifier);
        if (*qualifier == 2) {                  // fmi3IntervalChanged
            *interval = M(inClock3_interval);
            M(inClock3_qualifier) = 1;          // fmi3IntervalUnchanged
        }
        return OK;
    default:
        logError(comp, "Get Interval is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status activateModelPartition(ModelInstance* comp, ValueReference vr, double activationTime) {

    switch (vr) {
        case vr_inClock1:
            activateModelPartition1(comp, activationTime);
            return OK;
        case vr_inClock2:
            activateModelPartition2(comp, activationTime);
            return OK;
        case vr_inClock3:
            activateModelPartition3(comp, activationTime);
            return OK;
        default:
            logError(comp, "Activate model partition is not allowed for value reference %u.", vr);
            return Error;
    }
}
