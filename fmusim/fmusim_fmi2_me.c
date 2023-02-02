#include <stdlib.h>
#include <math.h>

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

    //// from XML file
    //const size_t nx = 2; // number of states
    //const fmi2ValueReference x_ref[2]  = { 1, 3 }; // vector of value references of cont.-time states
    //const fmi2ValueReference xd_ref[2] = { 2, 4 }; // vector of value references of state derivatives

    //const fmi2Real dvKnown = 1;
    //fmi2Real ci[2]; // auxiliary vector of nx elements
    //fmi2Real J[2][2];

    ////J[0][0] = 11; J[0][1] = 12;
    ////J[1][0] = 21; J[1][1] = 22;
    //
    //// Construct the Jacobian elements J[:,:] columnwise
    //for (size_t i = 0; i < nx; i++) {
    //    CALL(FMI2GetDirectionalDerivative(S, xd_ref, nx, &x_ref[i], 1, &dvKnown, ci));
    //    J[0][i] = ci[0];
    //    J[1][i] = ci[1];
    //}

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