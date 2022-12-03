#include <stdlib.h>
#include <math.h>

#include "fmusim_fmi1_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI1ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    FMIRecorder* result,
    const FMUStaticInput * input,
    const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    fmi1Real time = settings->startTime;

    bool stateEvent = false;
    fmi1Boolean inputEvent = fmi1False;
    fmi1Boolean timeEvent  = fmi1False;
    fmi1Boolean stepEvent  = fmi1False;

    fmi1Boolean statesChanged = fmi1False;

    Solver* solver = NULL;

    size_t nSteps = 0;
    fmi1Real nextRegularPoint;
    fmi1Real nextCommunicationPoint;
    fmi1Real nextInputEventTime;

    fmi1EventInfo eventInfo = {
        .iterationConverged = fmi1False,
        .stateValueReferencesChanged = fmi1False,
        .stateValuesChanged = fmi1False,
        .terminateSimulation = fmi1False,
        .upcomingTimeEvent = fmi1False,
        .nextEventTime = INFINITY
    };

    CALL(FMI1InstantiateModel(S,
        modelDescription->modelExchange->modelIdentifier,  // modelIdentifier
        modelDescription->instantiationToken,              // GUID
        fmi1False                                          // loggingOn
    ));

    // set start values
    CALL(applyStartValues(S, settings));

    CALL(FMIApplyInput(S, input, time,
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    // initialize
    CALL(FMI1Initialize(S, fmi1False, 0.0));

    solver = settings->solverCreate(S, modelDescription, input, time);

    if (!solver) {
        status = FMIError;
        goto TERMINATE;
    }

    for (;;) {

        CALL(FMISample(S, time, result));

        if (time >= settings->stopTime) {
            break;
        }
    
        nextRegularPoint = settings->startTime + (nSteps + 1) * settings->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = nextCommunicationPoint >= nextInputEventTime;

        timeEvent = nextCommunicationPoint >= eventInfo.nextEventTime;

        if (inputEvent || timeEvent) {
            nextCommunicationPoint = fmin(nextInputEventTime, eventInfo.nextEventTime);
        }

        CALL(settings->solverStep(solver, nextCommunicationPoint, &time, &stateEvent));

        CALL(FMI1SetTime(S, time));

        CALL(FMIApplyInput(S, input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        if (time == nextRegularPoint) {
            nSteps++;
        }

        CALL(FMI1CompletedIntegratorStep(S, &stepEvent));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            // record the values before the event
            CALL(FMISample(S, time, result));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            statesChanged = fmi1False;

            // event iteration
            do {
                CALL(FMI1EventUpdate(S, fmi1True, &eventInfo));

                if (eventInfo.terminateSimulation) {
                    goto TERMINATE;
                }

                statesChanged |= eventInfo.stateValuesChanged;

            } while (!eventInfo.iterationConverged);

            if (!eventInfo.upcomingTimeEvent) {
                eventInfo.nextEventTime = INFINITY;
            }

            settings->solverReset(solver, time);
        }

    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI1Terminate(S);

        if (terminateStatus != FMIFatal) {
            FMI1FreeModelInstance(S);
        }
    }

    if (solver) {
        settings->solverFree(solver);
    }

    return status;
}
