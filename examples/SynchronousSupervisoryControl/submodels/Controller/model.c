#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(r) = false;       // Clock
    M(xr) = 0.0;                    // Sample
    M(ur) = 1.0;                    // Discrete state/output
    M(pre_ur) = 0.0;                // Previous ur
    M(ar) = 0.0;                    // Local var
}

void calculateValues(ModelInstance *comp) {
    UNUSED(comp)
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_xr:
            value[(*index)++] = M(xr);
            return OK;
        case vr_ur:
            value[(*index)++] = M(ur);
            return OK;
        case vr_pre_ur:
            value[(*index)++] = M(pre_ur);
            return OK;
        case vr_ar:
            value[(*index)++] = M(ar);
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_xr:
            M(xr) = value[(*index)++];
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
