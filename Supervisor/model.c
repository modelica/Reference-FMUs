#include <math.h>   // for fabs()
#include "config.h"
#include "model.h"

void setStartValues(ModelInstance *comp) {
    M(s) = false;       // Clock
    M(x) = 0.0;         // Sample
    M(as) = 1.0;        // Discrete state/output
    M(as_previous) = 1.0;
    M(clock_s_ticking) = false; // State Event
    M(z) = 0.0;
    M(pz) = 0.0; 
    // The following is suggested by Masoud.
    M(pz) = 2.0 - M(x);
}

Status calculateValues(ModelInstance *comp) {
    UNUSED(comp);
    M(z) = 2.0 - M(x);
    return OK;
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double values[], size_t nValues, size_t* index) {
    UNUSED(nValues);
    switch (vr) {
        case vr_as:
            if (comp->state == EventMode && M(clock_s_ticking)) {
                // We need this here because when clock s is ticking, we need to output the next state already
                //   (because the clocked partition depends on that value),
                //   without executing the state transition.
                // Therefore we compute the next state "M(as_previous) * -1.0" and output it.
                // The actual execution of the state transition happens in the eventUpdate function below.
                // See definition of clocked partition in 2.2.8.3. Model Partitions and Clocked Variables.
                values[(*index)++] = M(as_previous) * -1.0;
            }
            else {
                values[(*index)++] = M(as_previous);
            }
            return OK;
        case vr_x:
            values[(*index)++] = M(x);
            return OK;
        case vr_as_previous:
            values[(*index)++] = M(as_previous);
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
        comp->isDirtyValues = true;
        return OK;
    default:
        logError(comp, "Unexpected value reference: %d.", vr);
        return Error;
    }
}

Status getClock(ModelInstance* comp, ValueReference vr, bool* value) {

    // Do state event detection.
    // We assume that getClock is invoked before getFloat64,
    //  so the variable M(as) can take on the value corresponding to the event.
    // This is ok because as is part of the clocked partition of clock s,
    //  and therefore should not be queried when clock s is inactive.

    // The reason we check for the event crossing here is to conclude that
    //  we are in event mode because the clock s is about to tick.
    // This should ideally be done in EnterEventMode,
    //  but I do not know how to do it without changing the underlying framework.
    if (comp->state == EventMode && M(pz) * M(z) < 0.0 && !M(clock_s_ticking)) {
        M(clock_s_ticking) = true;
    }

    switch (vr) {
    case vr_s:
        if (comp->state == EventMode && M(clock_s_ticking)) {
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

void eventUpdate(ModelInstance* comp) {
    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation = false;
    comp->nextEventTimeDefined = false;

    if (M(clock_s_ticking)) {
        // Execute state transition
        M(as_previous) = M(as);
        M(as) = M(as_previous) * -1.0;
        M(clock_s_ticking) = false;

        // The following has to be done in order to ensure that,
        //   if the FMU remains in event mode and the getClock function gets called, it will return false.
        // This is correct as the state event should occur only once per super dense time instant.
        M(pz) = M(z);
    }
}

void getEventIndicators(ModelInstance* comp, double z[], size_t nz) {
    UNUSED(nz);
    if (comp->isDirtyValues) {
        // This has to be done since it's the only way we can be sure that, 
        //  when we enter event mode, we can check that it's because of the state event.
        calculateValues(comp);
        comp->isDirtyValues = true;
    }
    z[0] = M(z);
}
