#include <math.h>    // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(x) =  0.0;
    M(der_x) =  0.0;
    M(u) = 0.0;
}

Status calculateValues(ModelInstance *comp) {
    M(der_x) = - M(x) + M(u);
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    UNUSED(nValues);
    switch (vr) {
        case vr_x:
            values[(*index)++] = M(x);
            return OK;
        case vr_der_x:
            values[(*index)++] = M(der_x);
            return OK;
        case vr_u:
            values[(*index)++] = M(u);
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    UNUSED(nValues);
    switch (vr) {
        case vr_u:
            M(u) = values[(*index)++];
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
    UNUSED(nx);
    x[0] = M(x);
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(nx);
    M(x) = x[0];
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(nx);
    calculateValues(comp);
    dx[0] = M(der_x);
}
