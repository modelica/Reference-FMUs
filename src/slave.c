/****************************************************************
 *  Copyright (c) Dassault Systemes. All rights reserved.       *
 *  This file is part of the Test-FMUs. See LICENSE.txt in the  *
 *  project root for license information.                       *
 ****************************************************************/

#include "config.h"
#include "slave.h"
#include "solver.h"


// default implementations
#if NUMBER_OF_EVENT_INDICATORS < 1
void getEventIndicators(ModelInstance *comp, double z[], size_t nz) {
    // do nothing
}
#endif

#ifndef GET_FLOAT64
Status getFloat64(ModelInstance* comp, ValueReference vr, double *value, size_t *index) {
    return Error;
}
#endif

#ifndef GET_INT32
Status getInt32(ModelInstance* comp, ValueReference vr, int *value, size_t *index) {
    return Error;
}
#endif

#ifndef GET_BOOLEAN
Status getBoolean(ModelInstance* comp, ValueReference vr, bool *value, size_t *index) {
    return Error;
}
#endif

#ifndef GET_STRING
Status getString(ModelInstance* comp, ValueReference vr, const char **value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_FLOAT64
Status setFloat64(ModelInstance* comp, ValueReference vr, const double *value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_INT32
Status setInt32(ModelInstance* comp, ValueReference vr, const int *value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_BOOLEAN
Status setBoolean(ModelInstance* comp, ValueReference vr, const bool *value, size_t *index) {
    return Error;
}
#endif

#ifndef SET_STRING
Status setString(ModelInstance* comp, ValueReference vr, const char *const *value, size_t *index) {
    return Error;
}
#endif

#if NUMBER_OF_STATES < 1
void getContinuousStates(ModelInstance *comp, double x[], size_t nx) {}
void setContinuousStates(ModelInstance *comp, const double x[], size_t nx) {}
void getDerivatives(ModelInstance *comp, double dx[], size_t nx) {}
#endif

Status doStep(ModelInstance *comp, double t, double tNext) {

    int stateEvent = 0;
    int timeEvent  = 0;

    while (comp->time + FIXED_SOLVER_STEP < tNext) {
        
        solver_step(comp, comp->time, comp->time + FIXED_SOLVER_STEP, &comp->time, &stateEvent);

        // check for time event
        if (comp->nextEventTimeDefined && (comp->time >= comp->nextEventTime)) {
            timeEvent = 1;
        }

        if (stateEvent || timeEvent) {
            eventUpdate(comp);
            timeEvent  = 0;
            stateEvent = 0;
        }

        // terminate simulation, if requested by the model in the previous step
        if (comp->terminateSimulation) {
#if FMI_VERSION > 1
            comp->state = modelStepFailed;
#endif
            return Discard; // enforce termination of the simulation loop
        }

    }

    return OK;
}
