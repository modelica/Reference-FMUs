#include "model_instance.h"

#define COUNTER_MAX 10

Status setStartValues(ModelInstance *instance) {
    M(counter) = 1;

    instance->nextEventTime        = 1;
    instance->nextEventTimeDefined = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    UNUSED(instance);
    // nothing to do
    return OK;
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
    case vr_time:
        ASSERT_NVALUES(1);
        values[(*index)++] = instance->time;
        return OK;
    default:
        logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status getInt32(ModelInstance *instance, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
        case vr_counter:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(counter);
            return OK;
        default:
            logError(instance, "Get Int32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setInt32(ModelInstance *instance, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
    case vr_counter:
        if (instance->state != Instantiated && instance->state != InitializationMode) {
            logError(instance, "Variable \"counter\" can only be set in Instantiated and Intialization Mode.");
            return Error;
        }

        ASSERT_NVALUES(1);
        if (values[*index] > COUNTER_MAX) {
            logError(instance, "Variable \"counter\" must be at most %d but was %d.",
                COUNTER_MAX, values[*index]);
            return Error;
        }
        M(counter) = values[(*index)++];

        break;
    default:
        logError(instance, "Set Int32 is not allowed for value reference %u.", vr);
        return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status updateDiscreteStates(
    ModelInstance* instance,
    bool* discreteStatesNeedUpdate,
    bool* terminateSimulation,
    bool* nominalsOfContinuousStatesChanged,
    bool* valuesOfContinuousStatesChanged,
    bool* nextEventTimeDefined,
    double* nextEventTime
) {
    if (instance->nextEventTimeDefined && isClose(instance->time, instance->nextEventTime)) {
        M(counter)++;
        instance->nextEventTime += 1;
    }
    
    if (M(counter) > 10) {
        logError(instance, "Variable \"counter\" cannot be incremented for values >= 10.");
        return Error;
    }

    *nominalsOfContinuousStatesChanged = false;
    *terminateSimulation = false;
    *nextEventTimeDefined = false;

    return OK;
}

//Status eventUpdate(ModelInstance *instance) {
//    ASSERT_NOT_NULL2(instance);
//
//    if (instance->nextEventTimeDefined && isClose(instance->time, instance->nextEventTime)) {
//        M(counter)++;
//        instance->nextEventTime += 1;
//    }
//
//    if (M(counter) > 10) {
//        logError(instance, "Variable \"counter\" cannot be incremented for values >= 10.");
//        return Error;
//    }
//
//    instance->valuesOfContinuousStatesChanged   = false;
//    instance->nominalsOfContinuousStatesChanged = false;
//    instance->terminateSimulation               = M(counter) >= 10;
//    instance->nextEventTimeDefined              = true;
//
//    return OK;
//}
