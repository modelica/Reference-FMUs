#include <stdlib.h>
#include <math.h>

#include "FMIUtil.h"

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

    fmi2EventInfo eventInfo = { 
        .newDiscreteStatesNeeded           = fmi2False,
        .terminateSimulation               = fmi2False,
        .nominalsOfContinuousStatesChanged = fmi2False,
        .valuesOfContinuousStatesChanged   = fmi2False,
        .nextEventTimeDefined              = fmi2False,
        .nextEventTime                     = INFINITY
    };

    CALL(FMI2Instantiate(S,
        resourceURI,                          // fmuResourceLocation
        fmi2ModelExchange,                    // fmuType
        modelDescription->instantiationToken, // fmuGUID
        fmi2False,                            // visible
        fmi2False                             // loggingOn
    ));

    time = settings->startTime;

    if (settings->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, settings->initialFMUStateFile));
    }

    // set start values
    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, time,
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    if (!settings->initialFMUStateFile) {

        // initialize
        CALL(FMI2SetupExperiment(S, settings->tolerance > 0, settings->tolerance, time, fmi2False, 0));
        CALL(FMI2EnterInitializationMode(S));
        CALL(FMI2ExitInitializationMode(S));

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
    }

    solver = settings->solverCreate(S, modelDescription, input, settings->tolerance, time);

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

    if (settings->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, settings->finalFMUStateFile));
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI2Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal) {
        FMI2FreeInstance(S);
    }

    if (solver) {
        settings->solverFree(solver);
    }

    return status;
}