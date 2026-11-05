#include "config.h"
#include "model_instance.h"


Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    M(x) = 1.0;
    M(k) = 1.0;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    M(der_x) = -M(k) * M(x);

    instance->isDirtyValues = false;

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
        case vr_x:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(x);
            return OK;
        case vr_der_x:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(der_x);
            return OK;
        case vr_k:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(k);
            return OK;
        default:
            logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
        case vr_x:
            ASSERT_NVALUES(1);
            M(x) = values[(*index)++];
            break;
        case vr_k:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode) {
                logError(instance, "Variable k can only be set after instantiation or in initialization mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(k) = values[(*index)++];
            break;
        default:
            logError(instance, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getNumberOfContinuousStates(ModelInstance* instance, size_t* nContinuousStates) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nContinuousStates);
    *nContinuousStates = MAX_CONTINUOUS_STATES;
    return OK;
}

Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(instance);

    x[0] = M(x);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(instance);

    nominals[0] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    M(x) = x[0];

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(instance);

    dx[0] = M(der_x);

    return OK;
}
