#include "model.h"


Status setStartValues(ModelInstance *comp) {
    M(counter) = 1;

    comp->nextEventTime        = 1;
    comp->nextEventTimeDefined = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    UNUSED(comp);
    // nothing to do
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
    case vr_time:
        ASSERT_NVALUES(1);
        values[(*index)++] = comp->time;
        return OK;
    default:
        logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
        return Error;
    }
}

Status getInt32(ModelInstance* comp, ValueReference vr, int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
        case vr_counter:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(counter);
            return OK;
        default:
            logError(comp, "Get Int32 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setInt32(ModelInstance* comp, ValueReference vr, const int32_t values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
    case vr_counter:
        if (comp->state != Instantiated && comp->state != InitializationMode) {
            logError(comp, "Variable \"counter\" can only be set in Instantiated and Intialization Mode.");
            return Error;
        }

        if (values[*index] >= 10) {
            logError(comp, "The maximum value for variable \"counter\" is 10.");
            return Error;
        }

        ASSERT_NVALUES(1);
        M(counter) = values[(*index)++];

        break;
    default:
        logError(comp, "Set Int32 is not allowed for value reference %u.", vr);
        return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status eventUpdate(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    if (comp->nextEventTimeDefined && isClose(comp->time, comp->nextEventTime)) {
        M(counter)++;
        comp->nextEventTime += 1;
    }

    if (M(counter) > 10) {
        logError(comp, "Variable \"counter\" cannot be incremented for values >= 10.");
        return Error;
    }

    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = M(counter) >= 10;
    comp->nextEventTimeDefined              = true;

    return OK;
}
