#include "config.h"
#include "model_impl.h"


Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    M(x0) = 2;
    M(x1) = 0;
    M(mu) = 1;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL(instance);

    M(der_x0) = M(x1);
    M(der_x1) = M(mu) * ((1.0 - M(x0) * M(x0)) * M(x1)) - M(x0);

    instance->isDirtyValues = false;

    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(size);

    switch (vr) {
    case vr_time:
    case vr_x0:
    case vr_der_x0:
    case vr_x1:
    case vr_der_x1:
    case vr_mu:
        *size = 1;
        return OK;
    default:
        logError(instance, "Unknown value reference: %u.", vr);
        return Discard;
    }
}

Status getFloat64(ModelInstance *instance, ValueReference vr, double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_NOT_NULL(index);

    calculateValues(instance);

    switch (vr) {
        case vr_time:
            ASSERT_NVALUES(1);
            values[(*index)++] = instance->time;
            return OK;
        case vr_x0:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(x0);
            return OK;
        case vr_der_x0 :
            ASSERT_NVALUES(1);
            values[(*index)++] = M(der_x0);
            return OK;
        case vr_x1:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(x1);
            return OK;
        case vr_der_x1:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(der_x1);
            return OK;
        case vr_mu:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(mu);
            return OK;
        default:
            logError(instance, "Get Float64 is not allowed for value reference %u.", vr);
            return Discard;
    }
}

Status setFloat64(ModelInstance *instance, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(values);
    ASSERT_NOT_NULL(index);

    switch (vr) {
        case vr_x0:
            ASSERT_NVALUES(1);
            M(x0) = values[(*index)++];
            break;
        case vr_x1:
            ASSERT_NVALUES(1);
            M(x1) = values[(*index)++];
            break;
        case vr_mu:
            if (instance->type == ModelExchange &&
                instance->state != Instantiated &&
                instance->state != InitializationMode &&
                instance->state != EventMode) {
                logError(instance, "Variable mu can only be set after instantiation, in initialization mode or event mode.");
                return Discard;
            }
            ASSERT_NVALUES(1);
            M(mu) = values[(*index)++];
            break;
        default:
            logError(instance, "Set Float64 is not allowed for value reference %u.", vr);
            return Discard;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(x);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    x[0] = M(x0);
    x[1] = M(x1);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(nominals);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    nominals[0] = 1.0;
    nominals[1] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(x);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    M(x0) = x[0];
    M(x1) = x[1];

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(dx);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    dx[0] = M(der_x0);
    dx[1] = M(der_x1);

    return OK;
}

Status getPartialDerivative(ModelInstance *instance, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double *partialDerivative) {
    ASSERT_NOT_NULL(instance);
    ASSERT_NOT_NULL(partialDerivative);
    UNUSED(unknownIndex);
    UNUSED(knownIndex);

    if (unknown == vr_der_x0 && known == vr_x0) {
        *partialDerivative = 0;
    } else if (unknown == vr_der_x0 && known == vr_x1) {
        *partialDerivative = 1;
    } else if (unknown == vr_der_x1 && known == vr_x0) {
        *partialDerivative = -2 * M(x0) * M(x1) * M(mu) - 1;
    } else if (unknown == vr_der_x1 && known == vr_x1) {
        *partialDerivative = M(mu) * (1 - M(x0) * M(x0));
    } else if (unknown == vr_der_x1 && known == vr_mu && instance->state == InitializationMode) {
        *partialDerivative = (1 - M(x0) * M(x0)) * M(x1);
    } else {
        *partialDerivative = 0;
    }

    return OK;
}
