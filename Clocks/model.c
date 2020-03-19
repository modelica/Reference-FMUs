#include "config.h"
#include "model.h"
#include <math.h>
#include <stdlib.h>

/*
 
time        0 1 2 3 4 5 6 7 8 9
inClock1    +       +       +     t % 4 == 0
inClock2    + +             + +   t % 8 == 0 || (t - 1) % 8 == 0
inClock3                    +     outClock2
outClock1                   +     totalInClockTicks == 5
outClock2   +               +     t % 8 == 0
time        0 1 2 3 4 5 6 7 8 9

*/

static void activateModelPartition1(ModelInstance *comp, double time) {

    comp->lockPreemtion();
    
    // increment the counters
    M(inClock1Ticks)++;
    M(totalInClockTicks)++;
    
    // set the output clocks
    M(outClock1) = ((int)time) % 8 == 0;
    M(outClock2) = M(totalInClockTicks) % 5 == 0;
    
    comp->unlockPreemtion();

    if (M(outClock1) || M(outClock2)) {

        comp->intermediateUpdate(
            comp->componentEnvironment, // instanceEnvironment
            time,                       // intermediateUpdateTime
            0,                          // eventOccurred
            1,                          // clocksTicked
            0,                          // intermediateVariableSetAllowed
            0,                          // intermediateVariableGetAllowed
            1,                          // intermediateStepFinished
            0                           // canReturnEarly
        );
        
    }
}

static void activateModelPartition2(ModelInstance *comp, double time) {
    
    comp->lockPreemtion();

    // increment the counters
    M(inClock2Ticks)++;
    M(totalInClockTicks)++;
    
    // set the output clocks
    M(outClock1) = 0;
    M(outClock2) = M(totalInClockTicks) % 5 == 0;
    
    comp->unlockPreemtion();

    if (M(outClock2)) {

        comp->intermediateUpdate(
            comp->componentEnvironment, // instanceEnvironment
            time,                       // intermediateUpdateTime
            0,                          // eventOccurred
            1,                          // clocksTicked
            0,                          // intermediateVariableSetAllowed
            0,                          // intermediateVariableGetAllowed
            1,                          // intermediateStepFinished
            0                           // canReturnEarly
        );
        
    }
}

static void activateModelPartition3(ModelInstance *comp, double time) {

    comp->lockPreemtion();

    // increment the counters
    M(inClock3Ticks)++;
    M(totalInClockTicks)++;
    
    // set the output clocks
    M(outClock1) = 0;
    M(outClock2) = M(totalInClockTicks) % 5 == 0;
    
    comp->unlockPreemtion();

    if (M(outClock2)) {

        comp->intermediateUpdate(
            comp->componentEnvironment, // instanceEnvironment
            time,                       // intermediateUpdateTime
            0,                          // eventOccurred
            1,                          // clocksTicked
            0,                          // intermediateVariableSetAllowed
            0,                          // intermediateVariableGetAllowed
            1,                          // intermediateStepFinished
            0                           // canReturnEarly
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
