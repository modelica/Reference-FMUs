#include "config.h"
#include "model.h"


Status setStartValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(x) = 1.0;
    M(k) = 1.0;

    comp->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(der_x) = -M(k) * M(x);

    comp->isDirtyValues = false;

    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_SIZE_T(nValues, 1);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
        case vr_time:
            values[(*index)++] = comp->time;
            return OK;
        case vr_x:
            values[(*index)++] = M(x);
            return OK;
        case vr_der_x:
            values[(*index)++] = M(der_x);
            return OK;
        case vr_k:
            values[(*index)++] = M(k);
            return OK;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_SIZE_T(nValues, 1);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_x:
            M(x) = values[(*index)++];
            break;
        case vr_k:
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode) {
                logError(comp, "Variable k can only be set after instantiation or in initialization mode.");
                return Error;
            }
            M(k) = values[(*index)++];
            break;
        default:
            logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return MAX_CONTINUOUS_STATES;
}

Status getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    x[0] = M(x);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance* comp, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    nominals[0] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    M(x) = x[0];

    comp->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    dx[0] = M(der_x);

    return OK;
}
