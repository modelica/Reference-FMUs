#include <math.h>   // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(s) = false;       // Clock
    M(x) = 0.0;         // Sample
    M(as) = 1.0;        // Discrete state/output
    M(stateEvent) = false; // State Event
    M(z) = 0.0;
    M(pz) = 0.0;
}

Status calculateValues(ModelInstance *comp) {
    UNUSED(comp);
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    switch (vr) {
        case vr_as:
            if (comp->state == EventMode && M(stateEvent)) {
                value[(*index)++] = M(as)*-1.0;
            }
            else {
                value[(*index)++] = M(as);
            }
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
    
    // Do state event detection.
    // It's important that getClock is invoked before getFloat64, 
    //  so the variable M(as) can take on the value corresponding to the event.
    // This is ok because as is part of the clocked partition of clock s, 
    //  and therefore should not be queried when clock s is inactive.
    if (M(pz) * M(z) < 0.0 && !M(stateEvent)) {
        M(stateEvent) = true;
    }

    switch (vr) {
        case vr_s:
            if (comp->state == EventMode && M(stateEvent)) {
                (*value) = true;
            }
            else {
                (*value) = false;
            }
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

    if (M(stateEvent)) {
        // Execute state transition
        M(as) = M(as) * -1.0;
        M(stateEvent) = false;
    }
}

void getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    UNUSED(nz);
    M(pz) = M(z);
    M(z) = 2.0 - M(x);
    z[0] = M(z);
}
