#include "config.h"
#include "model.h"


void setStartValues(ModelInstance *comp) {
    M(x0) = 2;
    M(x1) = 0;
    M(mu) = 1;
}

Status calculateValues(ModelInstance *comp) {
    M(der_x0) = M(x1);
    M(der_x1) = M(mu) * ((1.0 - M(x0) * M(x0)) * M(x1)) - M(x0);
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    calculateValues(comp);

    switch (vr) {
        case vr_time:
            values[(*index)++] = comp->time;
            return OK;
        case vr_x0:
            values[(*index)++] = M(x0);
            return OK;
        case vr_der_x0 :
            values[(*index)++] = M(der_x0);
            return OK;
        case vr_x1:
            values[(*index)++] = M(x1);
            return OK;
        case vr_der_x1:
            values[(*index)++] = M(der_x1);
            return OK;
        case vr_mu:
            values[(*index)++] = M(mu);
            return OK;
        default:
            logError(comp, "Get Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_x0:
            M(x0) = values[(*index)++];
            return OK;
        case vr_x1:
            M(x1) = values[(*index)++];
            return OK;
        case vr_mu:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable mu can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
#endif
            M(mu) = values[(*index)++];
            return OK;
        default:
            logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return 2;
}

Status getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(nx);
    x[0] = M(x0);
    x[1] = M(x1);
    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(nx);
    M(x0) = x[0];
    M(x1) = x[1];
    calculateValues(comp);
    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(nx);
    calculateValues(comp);
    dx[0] = M(der_x0);
    dx[1] = M(der_x1);
    return OK;
}

#if FMI_VERSION > 1
Status getPartialDerivative(ModelInstance *comp, ValueReference unknown, ValueReference known, double *partialDerivative) {

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
#endif

Status eventUpdate(ModelInstance *comp) {

    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;

    return OK;
}
