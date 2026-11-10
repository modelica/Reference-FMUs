#include "model_impl.h"

#define COUNTER_MAX 10

Status setStartValues(ModelInstance *instance) {
    M(counter) = 1;

    instance->nextEventTime = 1;
    instance->nextEventTimeDefined = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    UNUSED(instance);
    // nothing to do
    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);

    switch (vr) {
    case vr_time:
    case vr_counter:
        *size = 1;
        return OK;
    default:
        logError(instance, "Unknown value reference: %u.", vr);
        return Discard;
    }
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues, size_t *index) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_NOT_NULL(index);

    calculateValues(instance);

    switch (vr) {
    case vr_time:
        ASSERT_NVALUES(1);
        values[(*index)++] = instance->time;
        return OK;
    default:
        logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status getInt32(ModelInstance *instance, ValueReference vr, int32_t values[], size_t nValues, size_t *index) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_NOT_NULL(index);

    calculateValues(instance);

    switch (vr) {
    case vr_counter:
        ASSERT_NVALUES(1);
        values[(*index)++] = M(counter);
        return OK;
    default:
        logError(instance, "Get Int32 is not allowed for value reference %u.", vr);
        return Discard;
    }
}

Status setInt32(ModelInstance *instance, ValueReference vr, const int32_t values[], size_t nValues, size_t *index) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_NOT_NULL(index);

    switch (vr) {
    case vr_counter:
        if (instance->state != Instantiated && instance->state != InitializationMode) {
            logError(instance, "Variable \"counter\" can only be set in Instantiated and Intialization Mode.");
            return Discard;
        }

        ASSERT_NVALUES(1);
        if (values[*index] > COUNTER_MAX) {
            logError(instance, "Variable \"counter\" must be at most %d but was %d.",
                     COUNTER_MAX, values[*index]);
            return Discard;
        }
        M(counter) = values[(*index)++];

        break;
    default:
        logError(instance, "Set Int32 is not allowed for value reference %u.", vr);
        return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status updateDiscreteStates(
    ModelInstance *instance,
    bool *discreteStatesNeedUpdate,
    bool *terminateSimulation,
    bool *nominalsOfContinuousStatesChanged,
    bool *valuesOfContinuousStatesChanged,
    bool *nextEventTimeDefined,
    double *nextEventTime) {
    if (isClose(instance->time, (double)M(counter)) || instance->time > M(counter)) {
        M(counter)
        ++;

        if (M(counter) > 10) {
            logError(instance, "Variable \"counter\" cannot be incremented for values > 10.");
            return Discard;
        }
    }

    *discreteStatesNeedUpdate = false;
    *terminateSimulation = M(counter) >= 10;
    *nominalsOfContinuousStatesChanged = false;
    *valuesOfContinuousStatesChanged = false;
    *nextEventTimeDefined = true;
    *nextEventTime = M(counter);

    return OK;
}
