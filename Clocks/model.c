#include "config.h"
#include "model_impl.h"
#include <math.h>
#include <stdlib.h>

/*

time        0 1 2 3 4 5 6 7 8 9
inClock1    + + + + + + + + + +   input,  triggered by the simulation algorithm every second
inClock2    + +             + +   input,  triggered by the simulation algorithm at 0, 1, 8 and 9
inClock3            +             input,  triggered by inClock1
outClock    ? ? ? ? ? ? ? ? ? ?   output, triggered by all inClocks (if totalInTicks % 5 == 0)
time        0 1 2 3 4 5 6 7 8 9

*/

/**************************************
ModelPartition 1 does the following:
  - increments the clock tick counters
  - if time is 4, countdown clock inClock3's interval is set (which will trigger Model Partition 3)
  - triggers outClock, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition1(ModelInstance *instance, double time) {

    /* Note: model partition of highest priority
        -> no interruption by other model partitions possible
        -> no lock preemption needed
    */

    // increment the counters
    M(inClock1Ticks)++;
    M(totalInClockTicks)++;

    // set countdown and output clocks
    if ((int)time == 4) {
        M(inClock3_qualifier)= 2; // fmi3IntervalChanged
        M(inClock3_interval) = 0.0;
    }

    M(outClock) = ((M(outClock) == false) && (M(totalInClockTicks) % 5 == 0));

    if (M(inClock3_qualifier) == 2 || M(outClock)) {
        instance->clockUpdate(instance->componentEnvironment);
    }
}

/**************************************
ModelPartition 2 does the following:
  - increments the clock tick counters
  - gets an input value
  - triggers outClock, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition2(ModelInstance *instance, double time) {

    UNUSED(time);

    if (instance->lockPreemption) {
        instance->lockPreemption();
    }

    // increment the counters
    M(inClock2Ticks)++;
    M(totalInClockTicks)++;

    M(result2) += M(input2);
    M(input2) = 0;

    // set output clocks
    M(outClock) = ((M(outClock) == false) && (M(totalInClockTicks) % 5 == 0));

    if (instance->unlockPreemption) {
        instance->unlockPreemption();
    }

    if (M(outClock)) {
        instance->clockUpdate(instance->componentEnvironment);
    }
}

/**************************************
ModelPartition 3 does the following:
  - increments the clock tick counters
  - burns some CPU cycles by calculating some nonsense
  - sets an output variable that is supposed to find its way into ModelPartition 2
  - triggers outClock, if the number of totalInTicks is a multiple of 5
**************************************/
static void activateModelPartition3(ModelInstance *instance, double time) {

    UNUSED(time);

    // increment the counters
    M(inClock3Ticks)++;

    // This partition is supposed to consume a bit of time on a low prio ...
    unsigned long sum = 0;
    for (int loop = 1; loop < 1000000000; loop++) {
        sum += loop;
    }
    (void)sum; // use variable to avoid compiler warnings

    M(output3) = 1000;

    if (instance->lockPreemption) {
        instance->lockPreemption();
    }

    M(totalInClockTicks)++;
    M(outClock) = ((M(outClock) == false) && (M(totalInClockTicks) % 5 == 0));

    if (instance->unlockPreemption) {
        instance->unlockPreemption();
    }

    if (M(outClock)) {
        instance->clockUpdate(instance->componentEnvironment);
    }
}

Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    M(inClock3_interval)  = 0.0;
    M(inClock3_qualifier) = 0; // fmi3IntervalNotYetKnown
    M(outClock)           = 0;
    M(inClock1Ticks)      = 0;
    M(inClock2Ticks)      = 0;
    M(inClock3Ticks)      = 0;
    M(totalInClockTicks)  = 0;
    M(result2)            = 0;
    M(input2)             = 0;
    M(output3)            = 0;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {

    UNUSED(instance);

    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);

    switch (vr) {
    case vr_time:
    case vr_inClock1:
    case vr_inClock2:
    case vr_inClock3:
    case vr_outClock1:
    case vr_outClock:
    case vr_inClock1Ticks:
    case vr_inClock2Ticks:
    case vr_inClock3Ticks:
    case vr_totalInClockTicks:
    case vr_result2:
    case vr_input2:
    case vr_output3:
        *size = 1;
        return OK;
    default:
        logError(instance, "Unknown value reference: %u.", vr);
        return Discard;
    }
}

Status eventUpdate(ModelInstance *instance) {
    UNUSED(instance);
    return OK;
}

Status activateClock(ModelInstance *instance, ValueReference vr) {

    UNUSED(instance);
    UNUSED(vr);

    return OK;
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
    case vr_time:
        values[(*index)++] = instance->time;
        return OK;
    default:
        logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status setInt32(ModelInstance *instance, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
    case vr_input2:
        M(input2) = values[(*index)++];
        return OK;
    default:
        logError(instance, "Set Int32 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInt32(ModelInstance *instance, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
    case vr_inClock1Ticks:
        values[(*index)++] = M(inClock1Ticks);
        return OK;
    case vr_inClock2Ticks:
        values[(*index)++] = M(inClock2Ticks);
        return OK;
    case vr_inClock3Ticks:
        values[(*index)++] = M(inClock3Ticks);
        return OK;
    case vr_totalInClockTicks:
        values[(*index)++] = M(totalInClockTicks);
        return OK;
    case vr_result2:
        values[(*index)++] = M(result2);
        return OK;
    case vr_input2:
        values[(*index)++] = M(input2);
        return OK;
    case vr_output3:
        values[(*index)++] = M(output3);
        return OK;
    default:
        logError(instance, "Get Int32 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getClock(ModelInstance *instance, ValueReference vr, bool* value) {

    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(value);

    switch (vr) {
    case vr_outClock:
        *value = M(outClock);
        M(outClock) = false;
        return OK;
    default:
        logError(instance, "Get Clock is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInterval(ModelInstance *instance, ValueReference vr, double* interval, int* qualifier) {
    switch (vr) {
    case vr_inClock3:
        *qualifier = M(inClock3_qualifier);
        if (*qualifier == 2) {                  // fmi3IntervalChanged
            *interval = M(inClock3_interval);
            M(inClock3_qualifier) = 1;          // fmi3IntervalUnchanged
        }
        return OK;
    default:
        logError(instance, "Get Interval is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status activateModelPartition(ModelInstance *instance, ValueReference vr, double activationTime) {

    switch (vr) {
        case vr_inClock1:
            activateModelPartition1(instance, activationTime);
            return OK;
        case vr_inClock2:
            activateModelPartition2(instance, activationTime);
            return OK;
        case vr_inClock3:
            activateModelPartition3(instance, activationTime);
            return OK;
        default:
            logError(instance, "Activate model partition is not allowed for value reference %u.", vr);
            return Discard;
    }
}
