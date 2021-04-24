#include <math.h>  // for fabs()
#include "config.h"
#include "model.h"

// shorthand to access the variables
#define M(v) (comp->modelData->v)

#define V_MIN (0.1)

void setStartValues(ModelInstance *comp) {
    M(h) =  1;
    M(v) =  0;
    M(g) = -9.81;
    M(e) = 0.7;
    M(ground) = 0.2;
}

void calculateValues(ModelInstance *comp) {
    UNUSED(comp)
    // do nothing
}

Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t* index) {
    switch (vr) {
        case vr_h:
            value[(*index)++] = M(h);
            return OK;
        case vr_der_h:
        case vr_v:
            value[(*index)++] = M(v);
            return OK;
        case vr_der_v:
        case vr_g:
            value[(*index)++] = M(g);
            return OK;
        case vr_e:
            value[(*index)++] = M(e);
            return OK;
        case vr_ground:
            value[(*index)++] = M(ground);
            return OK;
        case vr_v_min:
            value[(*index)++] = V_MIN;
            return OK;
        default:
            return Error;
    }
}

Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    switch (vr) {

        case vr_h:
            M(h) = value[(*index)++];
            return OK;

        case vr_v:
            M(v) = value[(*index)++];
            return OK;

        case vr_g:
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode) {
                logError(comp, "Variable g can only be set after instantiation or in initialization mode.");
                return Error;
            }
            M(g) = value[(*index)++];
            return OK;

        case vr_e:
            if (comp->type == ModelExchange &&
                comp->state != Instantiated &&
                comp->state != InitializationMode &&
                comp->state != EventMode) {
                logError(comp, "Variable e can only be set after instantiation, in initialization mode or event mode.");
                return Error;
            }
            M(e) = value[(*index)++];
            return OK;

        case vr_v_min:
            logError(comp, "Variable v_min (value reference %d) is constant and cannot be set.", vr_v_min);
            return Error;

        default:
            logError(comp, "Unexpected value reference: %.", vr);
            return Error;
    }
}

void eventUpdate(ModelInstance *comp) {
    /*
    Note that, if there's a zero crossing at the same time as the timed event,
    then the time event takes priority and that means that the zero crossing event will not occurr.
    This is a good example of an importer going into event mode for two events, and only finding one.
    */

    // Check for previously scheduled time events
    if (comp->timeEvent) {
        logEvent(comp, "Executed timed event at time %lf\n", comp->time);
        // Move the ground
        M(ground) = M(ground) - 0.1;
        // Prevent the ground from being moved in subsequent event iterations (there are none, but just to make the point).
        comp->timeEvent = false;
        // Signal Importer that there is not next time event.
        comp->nextEventTimeDefined = false;
    }

    // Check for bouncing on the ground.
    if (M(h) <= M(ground) && M(v) < 0) {
        logEvent(comp, "Bounce detected at %lf\n", comp->time);

        M(h) = M(ground);
        M(v) = -M(v) * M(e);

        if (M(v) < V_MIN) {
            // stop bouncing
            M(v) = 0;
            M(g) = 0;
        }

        // Each time a bounce occurs, we schedule an event to move the ground shortly after.
        comp->nextEventTimeDefined = true;
        comp->nextEventTime = comp->time + 0.2;
        logEvent(comp, "Scheduled next stair drop to: %lf\n", comp->nextEventTime);

        comp->valuesOfContinuousStatesChanged = true;
    }

    comp->nominalsOfContinuousStatesChanged = false;
    comp->terminateSimulation  = false;
}

void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {
    UNUSED(nx)
    x[0] = M(h);
    x[1] = M(v);
}

void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {
    UNUSED(nx)
    M(h) = x[0];
    M(v) = x[1];
}

void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {
    UNUSED(nx)
    dx[0] = M(v);
    dx[1] = M(g);
}

void getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    UNUSED(nz)
    z[0] = (M(h) == 0 && M(v) == 0) ? 1 : M(h)- M(ground);
}
