#include "config.h"
#include "model_instance.h"

#define Y1_NOMINAL (1e-4)
#define Y2_NOMINAL (1e-2)
#define Y3_NOMINAL (1e-2)

Status setStartValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    M(y1)  = 1.0;
    M(y2)  = 0.0;
    M(y3)  = 0.0;
    M(dae) = false;

    instance->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *instance) {
    ASSERT_NOT_NULL2(instance);

    if (!M(dae)) {
        M(y3) = 1.0 - M(y1) - M(y2);
    }

    M(der_y1) = (-0.04) * M(y1) + 1e4 * M(y2) * M(y3);
    M(der_y2) =  (0.04) * M(y1) - 1e4 * M(y2) * M(y3) - 3e7 * M(y2) * M(y2);

    M(r) = M(y1) + M(y2) + M(y3) - 1.0;

    instance->isDirtyValues = false;

    return OK;
}

Status configurate(ModelInstance* instance) {
    UNUSED(instance);
    return OK;
}

Status getVariableSize(ModelInstance *instance, ValueReference vr, size_t *size) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(size);

    switch (vr) {
    case vr_time:
    case vr_dae:
    case vr_y1:
    case vr_der_y1:
    case vr_y2:
    case vr_der_y2:
    case vr_y3:
    case vr_r:
    case vr_g1:
    case vr_g2:
        *size = 1;
        return OK;
    default:
        logError(instance, "Unknown value reference: %u.", vr);
        return Error;
    }
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
        case vr_y1:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(y1);
            return OK;
        case vr_der_y1:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(der_y1);
            return OK;
        case vr_y2:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(y2);
            return OK;
        case vr_der_y2:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(der_y2);
            return OK;
        case vr_y3:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(y3);
            return OK;
        case vr_r:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(r);
            return OK;
        case vr_g1:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(g1);
            return OK;
        case vr_g2:
            ASSERT_NVALUES(1);
            values[(*index)++] = M(g2);
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
    case vr_y1:
        ASSERT_NVALUES(1);
        M(y1) = values[(*index)++];
        break;
    case vr_y2:
        ASSERT_NVALUES(1);
        M(y2) = values[(*index)++];
        break;
    case vr_y3:
        ASSERT_NVALUES(1);
        M(y3) = values[(*index)++];
        break;
    default:
        logError(instance, "Unexpected value reference: %u.", vr);
        return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getBoolean(ModelInstance *instance, ValueReference vr, bool values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    calculateValues(instance);

    switch (vr) {
    case vr_dae:
        ASSERT_NVALUES(1);
        values[(*index)++] = M(dae);
        break;
    default:
        logError(instance, "Get Boolean is not allowed for value reference %u.", vr);
        return Error;
    }

    return OK;
}

Status setBoolean(ModelInstance *instance, ValueReference vr, const bool values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
    case vr_dae:
        ASSERT_NVALUES(1);
        M(dae) = values[(*index)++];
        break;
    default:
        logError(instance, "Set Boolean is not allowed for value reference %u.", vr);
        return Error;
    }

    instance->isDirtyValues = true;

    return OK;
}

Status getPartialDerivative(ModelInstance *instance, ValueReference unknown, size_t unknownIndex, ValueReference known, size_t knownIndex, double* partialDerivative) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(partialDerivative);
    UNUSED(unknownIndex);
    UNUSED(knownIndex);

    calculateValues(instance);

    if (unknown == vr_der_y1 && known == vr_y1) {
        *partialDerivative = -0.04;
    } else if (unknown == vr_der_y1 && known == vr_y2) {
        *partialDerivative = 1e4 * M(y3);
    } else if (unknown == vr_der_y1 && known == vr_y3) {
        *partialDerivative = 1e4 * M(y2);
    } else if (unknown == vr_der_y2 && known == vr_y1) {
        *partialDerivative = 0.04;
    } else if (unknown == vr_der_y2 && known == vr_y2) {
        *partialDerivative = -1e4 * M(y3) - 6e7 * M(y2);
    } else if (unknown == vr_der_y2 && known == vr_y3) {
        *partialDerivative = -1e4 * M(y2);
    } else if (unknown == vr_r && known == vr_y1) {
        *partialDerivative = 1.0;
    } else if (unknown == vr_r && known == vr_y2) {
        *partialDerivative = 1.0;
    } else if (unknown == vr_r && known == vr_y3) {
        *partialDerivative = 1.0;
    } else {
        *partialDerivative = 0.0;
    }

    return OK;
}

Status getNumberOfContinuousStates(ModelInstance* instance, size_t* nContinuousStates) {
    UNUSED(instance);
    ASSERT_NOT_NULL2(nContinuousStates);
    *nContinuousStates = N_CONTINUOUS_STATES;
    return OK;
}


Status getContinuousStates(ModelInstance *instance, double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    x[0] = M(y1);
    x[1] = M(y2);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance *instance, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    nominals[0] = Y1_NOMINAL;
    nominals[1] = Y2_NOMINAL;

    return OK;
}

Status setContinuousStates(ModelInstance *instance, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    M(y1) = x[0];
    M(y2) = x[1];

    instance->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *instance, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, N_CONTINUOUS_STATES);

    calculateValues(instance);

    dx[0] = M(der_y1);
    dx[1] = M(der_y2);

    return OK;
}

Status getEventIndicators(ModelInstance *instance, double z[], size_t nz) {
    ASSERT_NOT_NULL2(instance);
    ASSERT_NOT_NULL2(z);
    ASSERT_SIZE_T(nz, N_EVENT_INDICATORS);

    calculateValues(instance);

    z[0] = M(y1) - 0.0001;
    z[1] = M(y3) - 0.01;

    return OK;
}

Status getNumberOfEventIndicators(ModelInstance* instance, size_t* nEventIndicators) {
    UNUSED(instance);
    *nEventIndicators = N_EVENT_INDICATORS;
    return OK;
}
