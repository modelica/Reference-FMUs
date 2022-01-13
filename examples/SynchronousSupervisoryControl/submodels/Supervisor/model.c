#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(s) = fmi3ClockInactive;       // Clock
    M(x) = 0.0;                    // Sample
    M(as) = 0.0;                    // Discrete state/output
}

void calculateValues(ModelInstance *comp) {
    UNUSED(comp)
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_as:
            value[(*index)++] = M(as);
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {
        case vr_x:
            M(x) = value[(*index)++];
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status getClock(ModelInstance* comp, ValueReference vr, bool* value) {
    switch (vr) {
        case vr_s:
            (*value) = M(s);
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

void getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    UNUSED(nz);
    z[0] = 4.0 - M(x);
}
