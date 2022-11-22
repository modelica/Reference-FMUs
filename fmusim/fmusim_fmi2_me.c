#include <stdlib.h>
#include <math.h>

#include "fmusim.h"
#include "fmusim_fmi2.h"
#include "fmusim_fmi2_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI2ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourceURI,
    FMIRecorder* result,
    const FMUStaticInput * input,
    const FMISimulationSettings* settings) {

    FMIStatus status = FMIOK;

    fmi2Real time;

    bool stateEvent = false;
    fmi2Boolean inputEvent = fmi2False;
    fmi2Boolean timeEvent  = fmi2False;
    fmi2Boolean stepEvent  = fmi2False;

    fmi2Boolean nominalsChanged = fmi2False;
    fmi2Boolean statesChanged = fmi2False;

    Solver* solver = NULL;

    size_t nSteps;
    fmi2Real nextRegularPoint;
    fmi2Real nextCommunicationPoint;
    fmi2Real nextInputEventTime;

    CALL(FMI2Instantiate(S,
        resourceURI,                          // fmuResourceLocation
        fmi2ModelExchange,                    // fmuType
        modelDescription->instantiationToken, // fmuGUID
        fmi2False,                            // visible
        fmi2False                             // loggingOn
    ));

    time = settings->startTime;

    // set start values
    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, time,
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    // initialize
    CALL(FMI2SetupExperiment(S, fmi2False, 0.0, time, fmi2True, settings->stopTime));
    CALL(FMI2EnterInitializationMode(S));
    CALL(FMI2ExitInitializationMode(S));

    fmi2EventInfo eventInfo = { 0 };

    // intial event iteration
    nominalsChanged = fmi2False;
    statesChanged = fmi2False;

    do {

        CALL(FMI2NewDiscreteStates(S, &eventInfo));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        nominalsChanged |= eventInfo.nominalsOfContinuousStatesChanged;
        statesChanged |= eventInfo.valuesOfContinuousStatesChanged;

    } while (eventInfo.newDiscreteStatesNeeded);

    if (!eventInfo.nextEventTimeDefined) {
        eventInfo.nextEventTime = INFINITY;
    }

    CALL(FMI2EnterContinuousTimeMode(S));

    solver = settings->solverCreate(S, modelDescription, input, time);

    if (!solver) {
        status = FMIError;
        goto TERMINATE;
    }

    nSteps = 0;

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

        CALL(FMI2SetTime(S, time));

        CALL(FMIApplyInput(S, input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        if (time == nextRegularPoint) {
            nSteps++;
        }

        CALL(FMI2CompletedIntegratorStep(S, fmi2True, &stepEvent, &eventInfo.terminateSimulation));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            // record the values before the event
            CALL(FMISample(S, time, result));

            CALL(FMI2EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            nominalsChanged = fmi2False;
            statesChanged = fmi2False;

            // event iteration
            do {
                CALL(FMI2NewDiscreteStates(S, &eventInfo));

                if (eventInfo.terminateSimulation) {
                    goto TERMINATE;
                }

                nominalsChanged |= eventInfo.nominalsOfContinuousStatesChanged;
                statesChanged |= eventInfo.valuesOfContinuousStatesChanged;

            } while (eventInfo.newDiscreteStatesNeeded);

            if (!eventInfo.nextEventTimeDefined) {
                eventInfo.nextEventTime = INFINITY;
            }

            // enter Continuous-Time Mode
            CALL(FMI2EnterContinuousTimeMode(S));

            settings->solverReset(solver, time);
        }

    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI2Terminate(S);

        if (terminateStatus != FMIFatal) {
            FMI2FreeInstance(S);
        }
    }

    if (solver) {
        settings->solverFree(solver);
    }

    return status;
}