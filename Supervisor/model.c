#include <math.h>   // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(s) = false;       // Clock
    M(x) = 0.0;         // Sample
    M(as) = 1.0;        // Discrete state/output
    M(stateEvent) = false;
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
    switch (vr) {
        case vr_s:
            if (comp->state == EventMode && M(stateEvent)) {
                (*value) = true;
            }
            else {
                (*value) = true;
            }
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

void enterEventMode(ModelInstance* comp, int stepEvent, int stateEvent, const int rootsFound[], size_t nEventIndicators, int timeEvent) {
    UNUSED(stepEvent);
    UNUSED(rootsFound);
    UNUSED(nEventIndicators);
    UNUSED(timeEvent);

    M(stateEvent) = stateEvent == 1;
}

void eventUpdate(ModelInstance *comp) {
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation  = false;
    comp->nextEventTimeDefined = false;

    if (M(stateEvent)) {
        // Execute state transition
        M(as) = M(as) * -1.0;
    }
}

void getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    UNUSED(nz);
    z[0] = 2.0 - M(x);
}
