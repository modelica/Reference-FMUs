#include "config.h"
#include "model.h"
#include <math.h>
#include <stdlib.h>

/*

time        0 1 2 3 4 5 6 7 8 9
inClock1    + + + + + + + + + +   t % 4 == 0
inClock2    + +             + +   t % 8 == 0 || (t - 1) % 8 == 0
inClock3          +               depends on outClock1
outClock1         +               time == 3 (triggered by inClock1)
outClock2   ? ? ? ? ? ? ? ? ? ?   totalInTicks % 5 == 0 (triggered by all inClocks)
time        0 1 2 3 4 5 6 7 8 9

*/

/**************************************
ModelPartition 1 does the following:
  - increments the clock tick counters
  - resets that value
  - if time is 4, output clock 1 is triggered (which will trigger Model Partition 3)
  - triggers outClock2, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition1(ModelInstance *comp, double time) {

    comp->lockPreemtion();

    // increment the counters
    M(inClock1Ticks)++;
    M(totalInClockTicks)++;

    // set the output clocks
    M(outClock1) = ((M(outClock1) == false) && (((int)time) == 4));
    M(outClock2) = ((M(outClock2) == false) && (M(totalInClockTicks) % 5 == 0));

    comp->unlockPreemtion();

    int earlyReturnRequested;
    double earlyReturnTime;

    if (M(outClock1) || M(outClock2)) {
        comp->intermediateUpdate(
            comp, // fmu instance
            time, // intermediateUpdateTime
            0,    // eventOccurred
            1,    // clocksTicked
            0,    // intermediateVariableSetAllowed
            0,    // intermediateVariableGetAllowed
            1,    // intermediateStepFinished
            0,     // canReturnEarly
            &earlyReturnRequested,
            &earlyReturnTime
        );
    }
}

/**************************************
ModelPartition 2 does the following:
  - increments the clock tick counters
  - gets an input value (from ModelPartition3)
  - resets that value
  - triggers outClock2, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition2(ModelInstance* comp, double time) {

    comp->lockPreemtion();

    // increment the counters
    M(inClock2Ticks)++;
    M(totalInClockTicks)++;
    // Every time the input value is set, we sum it up

    M(result2) += M(input2);  // add the output from mp3
    M(input2) = 0;    // then reset the value

    // Due to some conditions, trigger output clock 2
    M(outClock2) = ((M(outClock2) == false) && (M(totalInClockTicks) % 5 == 0));
    comp->unlockPreemtion();

    int earlyReturnRequested;
    double earlyReturnTime;

    if (M(outClock2)) {
        comp->intermediateUpdate(
            comp,  // fmu instance
            time,  // intermediateUpdateTime
            0,     // eventOccurred
            1,     // clocksTicked
            0,     // intermediateVariableSetAllowed
            0,     // intermediateVariableGetAllowed
            1,     // intermediateStepFinished
            0,     // canReturnEarly
            &earlyReturnRequested,
            &earlyReturnTime
        );
    }
}

 /**************************************
 ModelPartition 3 does the following:
   - increments the clock tick counters
   - burns some CPU cycles by calculating some nonsense
   - sets an output variable that is supposed to find its way into ModelPartition 2
   - triggers outClock2, if the number of totalInTicks is a multiple of 5
 **************************************/
 static void activateModelPartition3(ModelInstance *comp, double time) {

    comp->lockPreemtion();

    // increment the counters
    M(inClock3Ticks)++;
    comp->unlockPreemtion();

    // This partition is supposed to consume a bit of time on a low prio ...
    unsigned long sum = 0;
    for (int loop = 1; loop < 1000000000; loop++) {
        sum += loop;
    }
    // ... end of burning CPU cycles
    M(output3) = 1000;   // this is suposed to find its way into mp2
    M(totalInClockTicks)++;

    M(outClock2) = ((M(outClock2) == false) && (M(totalInClockTicks) % 5 == 0));
    if (M(outClock2)) {

        int earlyReturnRequested;
        double earlyReturnTime;

        comp->intermediateUpdate(
            comp,  // fmu instance
            time,  // intermediateUpdateTime
            0,     // eventOccurred
            1,     // clocksTicked
            0,     // intermediateVariableSetAllowed
            0,     // intermediateVariableGetAllowed
            1,     // intermediateStepFinished
            0,     // canReturnEarly
            &earlyReturnRequested,
            &earlyReturnTime
        );
    }
}

void setStartValues(ModelInstance *comp) {
    M(outClock1)         = 0;
    M(outClock2)         = 0;
    M(inClock1Ticks)     = 0;
    M(inClock2Ticks)     = 0;
    M(inClock3Ticks)     = 0;
    M(totalInClockTicks) = 0;
    M(result2)           = 0;
    M(input2)            = 0;
    M(output3)           = 0;
}

void calculateValues(ModelInstance *comp) {
    // nothing to do
}

void eventUpdate(ModelInstance *comp) {
    // nothing to do
}

Status activateClock(ModelInstance* comp, ValueReference vr) {
    return OK;
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int* value, size_t* index) {
    switch (vr) {
    case vr_input2:
        M(input2) = value[(*index)++];
        return OK;
    default:
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
        return Error;
    }
}

Status getClock(ModelInstance* comp, ValueReference vr, int* value) {

    switch (vr) {
    case vr_outClock1:
        *value = M(outClock1);
        return OK;
    case vr_outClock2:
        *value = M(outClock2);
        return OK;
    default:
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
            return Error;
    }
}
