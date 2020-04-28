#include "config.h"
#include "model.h"


void setStartValues(ModelInstance *comp) {
    M(x) = 1;
    M(k) = 1;
}

void calculateValues(ModelInstance *comp) {
    M(der_x) = -M(k) * M(x);
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    calculateValues(comp);
    switch (vr) {
        case vr_x:
            value[(*index)++] = M(x);
            return OK;
        case vr_der_x:
            value[(*index)++] = M(der_x);
            return OK;
        case vr_k:
            value[(*index)++] = M(k);
            return OK;
        default:
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_x:
            M(x) = value[(*index)++];
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
            M(k) = value[(*index)++];
            return OK;
        default:
            return Error;
    }
}

void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(nx)
    x[0] = M(x);
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(nx)
    M(x) = x[0];
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(nx)
    calculateValues(comp);
    dx[0] = M(der_x);
}

void eventUpdate(ModelInstance *comp) {
    comp->valuesOfContinuousStatesChanged   = false;
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation               = false;
    comp->nextEventTimeDefined              = false;
}
