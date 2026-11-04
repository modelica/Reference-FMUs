#include <string.h>
#include "config.h"
#include "cosimulation.h"
#include "model_instance.h"

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

Status doStep(
    ModelInstance* comp,
    double currentCommunicationPoint,
    double communicationStepSize,
    bool noSetFMUStatePriorToCurrentPoint,
    bool* eventHandlingNeeded,
    bool* terminateSimulation,
    bool* earlyReturn,
    double* lastSuccessfulTime) {
    UNUSED(noSetFMUStatePriorToCurrentPoint);

    //if (!isClose(currentCommunicationPoint, comp->nextCommunicationPoint)) {
    //    logError(comp, "Expected currentCommunicationPoint = %.16g but was %.16g.",
    //        comp->nextCommunicationPoint, currentCommunicationPoint);
    //    CALL(Error);
    //}

    if (communicationStepSize <= 0) {
        logError(comp, "Communication step size must be > 0 but was %.16g.", communicationStepSize);
        CALL(Error);
    }

    const double nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;

    if (comp->stopTimeDefined
        && nextCommunicationPoint > comp->stopTime
        && !isClose(nextCommunicationPoint, comp->stopTime)
        ) {
        logError(comp, "At communication point %.16g a step size of %.16g was requested but stop time is %.16g.",
            currentCommunicationPoint, communicationStepSize, comp->stopTime);
        CALL(Error);
    }

    bool nextCommunicationPointReached;

    *eventHandlingNeeded = false;
    *terminateSimulation = false;
    *earlyReturn = false;

    while (true) {

        const double nextSolverStepTime = comp->time + FIXED_SOLVER_STEP;

        nextCommunicationPointReached = nextSolverStepTime > nextCommunicationPoint && !isClose(nextSolverStepTime, nextCommunicationPoint);

        if (nextCommunicationPointReached || (*eventHandlingNeeded && comp->earlyReturnAllowed)) {
            break;
        }

#ifdef EVENT_UPDATE
        bool nominalsOfContinuousStatesChanged = false;
        bool valuesOfContinuousStatesChanged = false;
        bool nextEventTimeDefined = false;
        double nextEventTime = 0.0;

        if (*eventHandlingNeeded) {
            updateDiscreteStates(
                comp,
                eventHandlingNeeded,
                terminateSimulation,
                &nominalsOfContinuousStatesChanged,
                &valuesOfContinuousStatesChanged,
                &nextEventTimeDefined,
                &nextEventTime
            );
        }
#endif

        bool stateEvent = false;
        bool timeEvent = false;

        CALL(doFixedStep(comp, &stateEvent, &timeEvent));

#ifdef EVENT_UPDATE
        if (stateEvent || timeEvent) {

            if (comp->eventModeUsed) {
                *eventHandlingNeeded = true;
            }
            else {
                updateDiscreteStates(
                    comp,
                    eventHandlingNeeded,
                    terminateSimulation,
                    &nominalsOfContinuousStatesChanged,
                    &valuesOfContinuousStatesChanged,
                    &nextEventTimeDefined,
                    &nextEventTime
                );
#if MAX_EVENT_INDICATORS > 0
                CALL(getEventIndicators(comp, comp->prez, comp->nz));
#endif
            }

            if (comp->earlyReturnAllowed) {
                break;
            }
        }
#endif

        if (*terminateSimulation) {
            break;
        }
    }

    //*terminateSimulation = comp->terminateSimulation;
    *earlyReturn = comp->earlyReturnAllowed && !nextCommunicationPointReached;
    *lastSuccessfulTime = comp->time;

    if (nextCommunicationPointReached) {
        comp->nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;
    }
    else {
        comp->nextCommunicationPoint = comp->time;
    }

    return OK;
}
