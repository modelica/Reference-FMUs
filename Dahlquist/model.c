#include "config.h"
#include "model.h"


void setStartValues(ModelInstance *comp) {
    M(x) = 1;
    M(k) = 1;
}

Status calculateValues(ModelInstance *comp) {
    M(der_x) = -M(k) * M(x);
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {

    calculateValues(comp);

    ASSERT_NVALUES(1);

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

    ASSERT_NVALUES(1);

    switch (vr) {
        case vr_x:
            M(x) = values[(*index)++];
            return OK;
        case vr_k:
#if FMI_VERSION > 1
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode) {
                logError(comp, "Variable k can only be set after instantiation or in initialization mode.");
                return Error;
            }
#endif
            M(k) = values[(*index)++];
            return OK;
        default:
            logError(comp, "Set Float64 is not allowed for value reference %u.", vr);
            return Error;
    }
}

size_t getNumberOfContinuousStates(ModelInstance* comp) {
    UNUSED(comp);
    return 1;
}

Status getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(nx);
    x[0] = M(x);
    return OK;
}

Status setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(nx);
    M(x) = x[0];
    return OK;
}

Status getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(nx);
    calculateValues(comp);
    dx[0] = M(der_x);
    return OK;
}
