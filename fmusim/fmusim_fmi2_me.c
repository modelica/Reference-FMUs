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
    FMISimulationResult* result,
    const FMUStaticInput * input,
    const FMISimulationSettings* settings) {

    bool stateEvent = false;
    fmi2Boolean inputEvent = fmi2False;
    fmi2Boolean timeEvent  = fmi2False;
    fmi2Boolean stepEvent  = fmi2False;
    fmi2Boolean nominalsChanged = fmi2False;
    fmi2Boolean statesChanged = fmi2False;

    Solver* solver = NULL;

    FMIStatus status = FMIOK;

    CALL(FMI2Instantiate(S,
        resourceURI,                          // fmuResourceLocation
        fmi2ModelExchange,                    // fmuType
        modelDescription->instantiationToken, // fmuGUID
        fmi2False,                            // visible
        fmi2False                             // loggingOn
    ));

    fmi2Real time = settings->startTime;

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
    do {

        CALL(FMI2NewDiscreteStates(S, &eventInfo));

        if (eventInfo.terminateSimulation) {
            goto TERMINATE;
        }
    } while (eventInfo.newDiscreteStatesNeeded);

    CALL(FMI2EnterContinuousTimeMode(S));

    solver = settings->solverCreate(S, modelDescription, input, time);

    CALL(FMISample(S, time, result));

    while (time < settings->stopTime) {
    
        const fmi2Real nextTime = time + settings->outputInterval;

        const double nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = time >= nextInputEventTime;
        
        settings->solverStep(solver, nextTime, &time, &stateEvent);

        CALL(FMI2SetTime(S, time));

        // apply continuous inputs
        CALL(FMIApplyInput(S, input, time,
            false,  // discrete
            true,   // continous
            false   // after event
        ));

        // check for step event, e.g.dynamic state selection
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

                nominalsChanged |= eventInfo.nominalsOfContinuousStatesChanged;
                statesChanged   |= eventInfo.valuesOfContinuousStatesChanged;

                if (eventInfo.terminateSimulation) {
                    goto TERMINATE;
                }

            } while (eventInfo.newDiscreteStatesNeeded);

            // enter Continuous-Time Mode
            CALL(FMI2EnterContinuousTimeMode(S));

            settings->solverReset(solver, time);

            // record outputs after the event
            CALL(FMISample(S, time, result));
        }

        // record outputs
        CALL(FMISample(S, time, result));
    }

TERMINATE:

    if (solver) {
        settings->solverFree(solver);
    }

    return status;
}