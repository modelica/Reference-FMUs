#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(r) = false;       // Clock
    M(xr) = 0.0;                    // Sample
    M(ur) = 0.0;                    // Discrete state/output
    M(pre_ur) = 0.0;                // Previous ur
    M(as) = 1.0;                    // In var from Supervisor
    M(s) = false;       // Clock from Supervisor
}

Status calculateValues(ModelInstance *comp) {
    UNUSED(comp);
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_xr:
            value[(*index)++] = M(xr);
            return OK;
        case vr_ur:
            if (M(r) == true) {
                value[(*index)++] = M(ur) + M(as);
            }
            else {
                value[(*index)++] = M(ur);
            }
            return OK;
        case vr_pre_ur:
            value[(*index)++] = M(pre_ur);
            return OK;
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
        case vr_xr:
            M(xr) = value[(*index)++];
            return OK;
        case vr_as:
            M(as) = value[(*index)++];
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status activateClock(ModelInstance* comp, ValueReference vr) {
    switch (vr) {
        case vr_r:
            M(r) = true;
            return OK;
        case vr_s:
            M(s) = true;
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status getClock(ModelInstance* comp, ValueReference vr, bool* value) {
    switch (vr) {
        case vr_r:
            (*value) = M(r);
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
    
    // State transition
    logEvent(comp, "Controller clock state transition.");
    M(pre_ur) = M(ur);
    M(ur) = M(ur) + M(as);

    // Deactivate clocks
    M(r) = false;
    M(s) = false;

}
