#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(x) =  0.0;
    M(der_x) =  0.0;
    M(u) = 0.0;
}

void calculateValues(ModelInstance *comp) {
    M(der_x) = - M(x) + M(u);
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_x:
            value[(*index)++] = M(x);
            return OK;
        case vr_der_x:
            value[(*index)++] = M(der_x);
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_u:
            M(u) = value[(*index)++];
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation  = false;
    comp->nextEventTimeDefined = false;
}

void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    x[0] = M(x);
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    M(x) = x[0];
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    calculateValues(comp);
    dx[0] = M(der_x);
}
