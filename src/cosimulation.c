#include <string.h>
#include "config.h"
#include "cosimulation.h"
#include "model_impl.h"

#ifdef CALL
#undef CALL
#endif

#define CALL(f)                  \
    do {                         \
        const Status status = f; \
        if (status != OK)        \
            return status;       \
    } while (false)

Status doFixedStep(ModelInstance *instance, bool *stateEvent, bool *timeEvent) {

#if N_CONTINUOUS_STATES > 0
    if (instance->nx > 0) {

        CALL(getContinuousStates(instance, instance->x, instance->nx));
        CALL(getDerivatives(instance, instance->dx, instance->nx));

        // forward Euler step
        for (size_t i = 0; i < instance->nx; i++) {
            instance->x[i] += FIXED_SOLVER_STEP * instance->dx[i];
        }

        CALL(setContinuousStates(instance, instance->x, instance->nx));
    }
#endif

    instance->nSteps++;

    instance->time = instance->startTime + instance->nSteps * FIXED_SOLVER_STEP;

    // state event
    *stateEvent = false;

#if N_EVENT_INDICATORS > 0
    if (instance->nz > 0) {

        CALL(getEventIndicators(instance, instance->z, instance->nz));

        // check for zero-crossings
        for (size_t i = 0; i < instance->nz; i++) {
            *stateEvent |=
                (instance->prez[i] <= 0 && instance->z[i] > 0) ||
                (instance->prez[i] > 0 && instance->z[i] <= 0);
        }

        // remember the current event indicators
        memcpy(instance->prez, instance->z, instance->nz * sizeof(double));
    }
#endif

    // time event
    *timeEvent = instance->nextEventTimeDefined && instance->time >= instance->nextEventTime;

    bool earlyReturnRequested;
    double earlyReturnTime;

    // intermediate update
    if (instance->intermediateUpdate) {
        instance->intermediateUpdate(
            instance->componentEnvironment, // instanceEnvironment
            instance->time,                 // intermediateUpdateTime
            false,                          // intermediateVariableSetRequested
            true,                           // intermediateVariableGetAllowed
            true,                           // intermediateStepFinished
            false,                          // canReturnEarly
            &earlyReturnRequested,          // earlyReturnRequested
            &earlyReturnTime);              // earlyReturnTime
    }

    return OK;
}

Status doStep(
    ModelInstance *instance,
    double currentCommunicationPoint,
    double communicationStepSize,
    bool noSetFMUStatePriorToCurrentPoint,
    bool *eventHandlingNeeded,
    bool *terminateSimulation,
    bool *earlyReturn,
    double *lastSuccessfulTime) {
    UNUSED(noSetFMUStatePriorToCurrentPoint);

    if (!isClose(currentCommunicationPoint, instance->nextCommunicationPoint)) {
        logError(instance, "Expected currentCommunicationPoint = %.16g but was %.16g.",
                 instance->nextCommunicationPoint, currentCommunicationPoint);
        return Discard;
    }

    if (communicationStepSize <= 0) {
        logError(instance, "Communication step size must be > 0 but was %.16g.", communicationStepSize);
        return Discard;
    }

    const double nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;

    if (instance->stopTimeDefined && nextCommunicationPoint > instance->stopTime && !isClose(nextCommunicationPoint, instance->stopTime)) {
        logError(instance, "At communication point %.16g a step size of %.16g was requested but stop time is %.16g.",
                 currentCommunicationPoint, communicationStepSize, instance->stopTime);
        return Discard;
    }

    bool nextCommunicationPointReached;

    *eventHandlingNeeded = false;
    *terminateSimulation = false;
    *earlyReturn = false;

    while (true) {

        const double nextSolverStepTime = instance->time + FIXED_SOLVER_STEP;

        nextCommunicationPointReached = nextSolverStepTime > nextCommunicationPoint && !isClose(nextSolverStepTime, nextCommunicationPoint);

        if (nextCommunicationPointReached || (*eventHandlingNeeded && instance->earlyReturnAllowed)) {
            break;
        }

#ifdef EVENT_UPDATE
        bool nominalsOfContinuousStatesChanged = false;
        bool valuesOfContinuousStatesChanged = false;
        bool nextEventTimeDefined = false;
        double nextEventTime = 0.0;

        if (*eventHandlingNeeded) {
            updateDiscreteStates(
                instance,
                eventHandlingNeeded,
                terminateSimulation,
                &nominalsOfContinuousStatesChanged,
                &valuesOfContinuousStatesChanged,
                &nextEventTimeDefined,
                &nextEventTime);
        }
#endif

        bool stateEvent = false;
        bool timeEvent = false;

        CALL(doFixedStep(instance, &stateEvent, &timeEvent));

#ifdef EVENT_UPDATE
        if (stateEvent || timeEvent) {

            if (instance->eventModeUsed) {
                *eventHandlingNeeded = true;
            } else {
                updateDiscreteStates(
                    instance,
                    eventHandlingNeeded,
                    terminateSimulation,
                    &nominalsOfContinuousStatesChanged,
                    &valuesOfContinuousStatesChanged,
                    &nextEventTimeDefined,
                    &nextEventTime);
#if N_EVENT_INDICATORS > 0
                CALL(getEventIndicators(instance, instance->prez, instance->nz));
#endif
            }

            if (instance->earlyReturnAllowed) {
                break;
            }
        }
#endif

        if (*terminateSimulation) {
            break;
        }
    }

    *earlyReturn = instance->earlyReturnAllowed && !nextCommunicationPointReached;
    *lastSuccessfulTime = instance->time;

    if (nextCommunicationPointReached) {
        instance->nextCommunicationPoint = currentCommunicationPoint + communicationStepSize;
    } else {
        instance->nextCommunicationPoint = instance->time;
    }

    return OK;
}
