#include <string.h>
#include "config.h"
#include "cosimulation.h"

#ifdef CALL
#undef CALL
#endif

#define CALL(f) do { const Status status = f; if (status != OK) return status; } while (false)

Status doFixedStep(ModelInstance *comp, bool* stateEvent, bool* timeEvent) {

#if MAX_CONTINUOUS_STATES > 0
    if (comp->nx > 0) {

        CALL(getContinuousStates(comp, comp->x, comp->nx));
        CALL(getDerivatives(comp, comp->dx, comp->nx));

        // forward Euler step
        for (size_t i = 0; i < comp->nx; i++) {
            comp->x[i] += FIXED_SOLVER_STEP * comp->dx[i];
        }

        CALL(setContinuousStates(comp, comp->x, comp->nx));
    }
#endif

    comp->nSteps++;

    comp->time = comp->startTime + comp->nSteps * FIXED_SOLVER_STEP;

    // state event
    *stateEvent = false;

#if MAX_EVENT_INDICATORS > 0
    if (comp->nz > 0) {

        CALL(getEventIndicators(comp, comp->z, comp->nz));

        // check for zero-crossings
        for (size_t i = 0; i < comp->nz; i++) {
            *stateEvent |=
                (comp->prez[i] <= 0 && comp->z[i] >  0) ||
                (comp->prez[i] >  0 && comp->z[i] <= 0);
        }

        // remember the current event indicators
        memcpy(comp->prez, comp->z, comp->nz * sizeof(double));
    }
#endif

    // time event
    *timeEvent = comp->nextEventTimeDefined && comp->time >= comp->nextEventTime;

    bool earlyReturnRequested;
    double earlyReturnTime;

    // intermediate update
    if (comp->intermediateUpdate) {
        comp->intermediateUpdate(
            comp->componentEnvironment, // instanceEnvironment
            comp->time,                 // intermediateUpdateTime
            false,                      // intermediateVariableSetRequested
            true,                       // intermediateVariableGetAllowed
            true,                       // intermediateStepFinished
            false,                      // canReturnEarly
            &earlyReturnRequested,      // earlyReturnRequested
            &earlyReturnTime);          // earlyReturnTime
    }

    return OK;
}
