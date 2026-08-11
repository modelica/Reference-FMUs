#include "config.h"
#include "model.h"

#define Y1_NOMINAL (1e-4)
#define Y2_NOMINAL (1e-2)
#define Y3_NOMINAL (1e-2)

Status setStartValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(y1)  = 1.0;
    M(y2)  = 0.0;
    M(y3)  = 0.0;
    M(dae) = false;

    comp->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    if (!M(dae)) {
        M(y3) = 1.0 - M(y1) - M(y2);
    }

    M(der_y1) = (-0.04) * M(y1) + 1e4 * M(y2) * M(y3);
    M(der_y2) =  (0.04) * M(y1) - 1e4 * M(y2) * M(y3) - 3e7 * M(y2) * M(y2);

    M(r) = M(y1) + M(y2) + M(y3) - 1.0;

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
        case vr_y1:
            values[(*index)++] = M(y1);
            return OK;
        case vr_der_y1:
            values[(*index)++] = M(der_y1);
            return OK;
        case vr_y2:
            values[(*index)++] = M(y2);
            return OK;
        case vr_der_y2:
            values[(*index)++] = M(der_y2);
            return OK;
        case vr_y3:
            values[(*index)++] = M(y3);
            return OK;
        case vr_y3_nominal:
            values[(*index)++] = Y3_NOMINAL;
            return OK;
        case vr_r:
            values[(*index)++] = M(r);
            return OK;
        case vr_g1:
            values[(*index)++] = M(g1);
            return OK;
        case vr_g2:
            values[(*index)++] = M(g2);
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
    case vr_y1:
        M(y1) = values[(*index)++];
        break;
    case vr_y2:
        M(y2) = values[(*index)++];
        break;
    case vr_y3:
        M(y3) = values[(*index)++];
        break;
    default:
        logError(comp, "Unexpected value reference: %u.", vr);
        return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

Status getBoolean(ModelInstance* comp, ValueReference vr, bool values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_SIZE_T(nValues, 1);
    ASSERT_NOT_NULL2(index);

    calculateValues(comp);

    switch (vr) {
    case vr_dae:
        values[(*index)++] = M(dae);
        break;
    default:
        logError(comp, "Get Boolean is not allowed for value reference %u.", vr);
        return Error;
    }

    return OK;
}

Status setBoolean(ModelInstance* comp, ValueReference vr, const bool values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_SIZE_T(nValues, 1);
    ASSERT_NOT_NULL2(index);

    switch (vr) {
    case vr_dae:
        M(dae) = values[(*index)++];
        break;
    default:
        logError(comp, "Set Boolean is not allowed for value reference %u.", vr);
        return Error;
    }

    comp->isDirtyValues = true;

    return OK;
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return MAX_CONTINUOUS_STATES;
}

size_t getNumberOfEventIndicators(ModelInstance* comp) {
    UNUSED(comp);
    return MAX_EVENT_INDICATORS;
}

Status getPartialDerivative(ModelInstance* comp, ValueReference unknown, ValueReference known, double* partialDerivative) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(partialDerivative);

    calculateValues(comp);

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

Status getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    x[0] = M(y1);
    x[1] = M(y2);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance* comp, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    nominals[0] = Y1_NOMINAL;
    nominals[1] = Y2_NOMINAL;

    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    M(y1) = x[0];
    M(y2) = x[1];

    comp->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    dx[0] = M(der_y1);
    dx[1] = M(der_y2);

    return OK;
}

Status getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(z);
    ASSERT_SIZE_T(nz, MAX_EVENT_INDICATORS);

    calculateValues(comp);

    z[0] = M(y1) - 0.0001;
    z[1] = M(y3) - 0.01;

    return OK;
}
