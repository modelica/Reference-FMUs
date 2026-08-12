#include "config.h"
#include "model.h"


Status setStartValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(x0) = 2;
    M(x1) = 0;
    M(mu) = 1;

    comp->isDirtyValues = true;

    return OK;
}

Status calculateValues(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    M(der_x0) = M(x1);
    M(der_x1) = M(mu) * ((1.0 - M(x0) * M(x0)) * M(x1)) - M(x0);

    comp->isDirtyValues = false;

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
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(values);
    ASSERT_NOT_NULL2(index);

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
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable mu can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
            ASSERT_NVALUES(1);
            M(mu) = values[(*index)++];
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

    x[0] = M(x0);
    x[1] = M(x1);

    return OK;
}

Status getNominalsOfContinuousStates(ModelInstance* comp, double nominals[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(nominals);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    nominals[0] = 1.0;
    nominals[1] = 1.0;

    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(x);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    M(x0) = x[0];
    M(x1) = x[1];

    comp->isDirtyValues = true;

    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(dx);
    ASSERT_SIZE_T(nx, MAX_CONTINUOUS_STATES);

    calculateValues(comp);

    dx[0] = M(der_x0);
    dx[1] = M(der_x1);

    return OK;
}

Status getPartialDerivative(ModelInstance *comp, ValueReference unknown, ValueReference known, double *partialDerivative) {
    ASSERT_NOT_NULL2(comp);
    ASSERT_NOT_NULL2(partialDerivative);

    if (unknown == vr_der_x0 && known == vr_x0) {
        *partialDerivative = 0;
    } else if (unknown == vr_der_x0 && known == vr_x1) {
        *partialDerivative = 1;
    } else if (unknown == vr_der_x1 && known == vr_x0) {
        *partialDerivative = -2 * M(x0) * M(x1) * M(mu) - 1;
    } else if (unknown == vr_der_x1 && known == vr_x1) {
        *partialDerivative = M(mu) * (1 - M(x0) * M(x0));
    } else if (unknown == vr_der_x1 && known == vr_mu && comp->state == InitializationMode) {
        *partialDerivative = (1 - M(x0) * M(x0)) * M(x1);
    } else {
        *partialDerivative = 0;
    }

    return OK;
}

Status eventUpdate(ModelInstance *comp) {
    ASSERT_NOT_NULL2(comp);

    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;

    return OK;
}
