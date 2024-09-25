#include <stdlib.h>
#include <math.h>

#include "FMI1.h"
#include "FMI1MESimulation.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus FMI1MESimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

    FMIInstance* S = s->S;

    fmi1Real time = s->startTime;

    bool stateEvent = false;
    fmi1Boolean inputEvent = fmi1False;
    fmi1Boolean timeEvent  = fmi1False;
    fmi1Boolean stepEvent  = fmi1False;

    fmi1Boolean resetSolver;

    FMISolver* solver = NULL;

    size_t nSteps = 0;
    fmi1Real nextRegularPoint;
    fmi1Real nextCommunicationPoint;
    fmi1Real nextInputEventTime;

    fmi1EventInfo eventInfo = {
        .iterationConverged          = fmi1False,
        .stateValueReferencesChanged = fmi1False,
        .stateValuesChanged          = fmi1False,
        .terminateSimulation         = fmi1False,
        .upcomingTimeEvent           = fmi1False,
        .nextEventTime               = INFINITY
    };

    CALL(FMI1InstantiateModel(S,
        s->modelDescription->modelExchange->modelIdentifier,  // modelIdentifier
        s->modelDescription->instantiationToken,              // GUID
        s->loggingOn                                          // loggingOn
    ));

    // set start values
    CALL(FMIApplyStartValues(S, s));

    CALL(FMIApplyInput(S, s->input, time,
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    // initialize
    CALL(FMI1Initialize(S, s->tolerance > 0, s->tolerance, &eventInfo));

    if (!eventInfo.upcomingTimeEvent) {
        eventInfo.nextEventTime = INFINITY;
    }

    const FMISolverParameters solverFunctions = {
        .modelInstance = S,
        .input = s->input,
        .startTime = time,
        .tolerance = s->tolerance,
        .nx = s->modelDescription->nContinuousStates,
        .nz = s->modelDescription->nEventIndicators,
        .setTime = (FMISolverSetTime)FMI1SetTime,
        .applyInput = (FMISolverApplyInput)FMIApplyInput,
        .getContinuousStates = (FMISolverGetContinuousStates)FMI1GetContinuousStates,
        .setContinuousStates = (FMISolverSetContinuousStates)FMI1SetContinuousStates,
        .getNominalsOfContinuousStates = (FMISolverGetNominalsOfContinuousStates)FMI1GetNominalContinuousStates,
        .getContinuousStateDerivatives = (FMISolverGetContinuousStateDerivatives)FMI1GetDerivatives,
        .getEventIndicators = (FMISolverGetEventIndicators)FMI1GetEventIndicators,
        .logError = (FMISolverLogError)FMILogError
    };

    solver = s->solverCreate(&solverFunctions);

    if (!solver) {
        status = FMIError;
        goto TERMINATE;
    }

    CALL(FMISample(S, time, s->initialRecorder));

    for (;;) {

        CALL(FMISample(S, time, s->recorder));

        if (time > s->stopTime || FMIIsClose(time, s->stopTime)) {
            break;
        }
    
        nextRegularPoint = s->startTime + (nSteps + 1) * s->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(s->input, time);

        if (nextCommunicationPoint > nextInputEventTime && !FMIIsClose(nextCommunicationPoint, nextInputEventTime)) {
            nextCommunicationPoint = nextInputEventTime;
        }

        if (eventInfo.upcomingTimeEvent && nextCommunicationPoint > eventInfo.nextEventTime && !FMIIsClose(nextCommunicationPoint, eventInfo.nextEventTime)) {
            nextCommunicationPoint = eventInfo.nextEventTime;
        }

        inputEvent = FMIIsClose(nextCommunicationPoint, nextInputEventTime);

        timeEvent = eventInfo.upcomingTimeEvent && FMIIsClose(nextCommunicationPoint, eventInfo.nextEventTime);


        CALL(s->solverStep(solver, nextCommunicationPoint, &time, &stateEvent));

        CALL(FMI1SetTime(S, time));

        CALL(FMIApplyInput(S, s->input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        if (FMIIsClose(time, nextRegularPoint)) {
            nSteps++;
        }

        CALL(FMI1CompletedIntegratorStep(S, &stepEvent));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            // record the values before the event
            CALL(FMISample(S, time, s->recorder));

            if (inputEvent) {
                CALL(FMIApplyInput(S, s->input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            resetSolver = fmi1False;

            do {

                CALL(FMI1EventUpdate(S, fmi1True, &eventInfo));

                if (eventInfo.terminateSimulation) {
                    CALL(FMISample(S, time, s->recorder));
                    goto TERMINATE;
                }

                resetSolver |= eventInfo.stateValuesChanged;

            } while (!eventInfo.iterationConverged);

            if (resetSolver) {
                s->solverReset(solver, time);
            }
        }

        if (s->stepFinished && !s->stepFinished(s, time)) {
            break;
        }
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI1Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal) {
        FMI1FreeModelInstance(S);
    }

    if (solver) {
        s->solverFree(solver);
    }

    return status;
}
