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

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    UNUSED(nValues);
    switch (vr) {
        case vr_as:
            if (comp->state == EventMode && M(stateEvent)) {
                values[(*index)++] = M(as)*-1.0;
            }
            else {
                values[(*index)++] = M(as);
            }
            return OK;
        default:
            logError(comp, "Unexpected value reference: %d.", vr);
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double values[], size_t nValues, size_t* index) {
    UNUSED(nValues);
    switch (vr) {
        case vr_x:
            M(x) = values[(*index)++];
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

        // The following has to be done in order to ensure that,
        //   if the FMU remains in event mode and the getClock function gets called, it will return false.
        // This is correct as the state event should occurr only once per super dense time instant.
        M(pz) = M(z);
    }
}

void getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    UNUSED(nz);
    M(pz) = M(z);
    M(z) = 2.0 - M(x);
    z[0] = M(z);
}
