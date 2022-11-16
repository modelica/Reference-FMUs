#include <stdlib.h>
#include <math.h>
#include "fmusim_fmi3.h"
#include "fmusim_fmi3_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMISimulationResult* result,
    const FMUStaticInput * input,
    const FMISimulationSettings * settings) {

    FMIStatus status = FMIOK;

    fmi3Boolean inputEvent = fmi3False;
    fmi3Boolean timeEvent  = fmi3False;
    fmi3Boolean stateEvent = fmi3False;
    fmi3Boolean stepEvent  = fmi3False;

    fmi3Boolean discreteStatesNeedUpdate          = fmi3True;
    fmi3Boolean terminateSimulation               = fmi3False;
    fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
    fmi3Boolean valuesOfContinuousStatesChanged   = fmi3False;
    fmi3Boolean nextEventTimeDefined              = fmi3False;
    fmi3Float64 nextEventTime                     = INFINITY;
    fmi3Float64 nextInputEvent                    = INFINITY;

    Solver* solver = NULL;

    CALL(FMI3InstantiateModelExchange(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False                              // loggingOn
    ));

    fmi3Float64 time = settings->startTime;

    // set start values
    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, time,
        true,  // discrete
        true,  // continous
        false  // after event
    ));

    // initialize
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, settings->stopTime));
    CALL(FMI3ExitInitializationMode(S));

    // intial event iteration
    while (discreteStatesNeedUpdate) {

        CALL(FMI3UpdateDiscreteStates(S,
            &discreteStatesNeedUpdate,
            &terminateSimulation,
            &nominalsOfContinuousStatesChanged,
            &valuesOfContinuousStatesChanged,
            &nextEventTimeDefined,
            &nextEventTime));

        if (terminateSimulation) {
            goto TERMINATE;
        }
    }

    CALL(FMI3EnterContinuousTimeMode(S));

    solver = settings->solverCreate(S, modelDescription, input, time);

    CALL(FMISample(S, time, result));

    uint64_t step = 0;

    while (time < settings->stopTime) {

        const fmi3Float64 nextTime = time + settings->outputInterval;

        const double nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = time >= nextInputEventTime;

        timeEvent = nextEventTimeDefined && time >= nextEventTime;

        settings->solverStep(solver, nextTime, &time, &stateEvent);

        CALL(FMI3SetTime(S, time));

        CALL(FMIApplyInput(S, input, time,
            false, // discrete
            true,  // continous
            false  // after event
        ));

        CALL(FMI3CompletedIntegratorStep(S, fmi3True, &stepEvent, &terminateSimulation));

        if (inputEvent || timeEvent || stateEvent || stepEvent) {

            CALL(FMISample(S, time, result));

            CALL(FMI3EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            nominalsOfContinuousStatesChanged = fmi3False;
            valuesOfContinuousStatesChanged = fmi3False;

            do {
                fmi3Boolean nominalsChanged = fmi3False;
                fmi3Boolean statesChanged = fmi3False;

                CALL(FMI3UpdateDiscreteStates(S, &discreteStatesNeedUpdate, &terminateSimulation, &nominalsChanged, &statesChanged, &nextEventTimeDefined, &nextEventTime));

                nominalsOfContinuousStatesChanged |= nominalsChanged;
                valuesOfContinuousStatesChanged |= statesChanged;

                if (terminateSimulation) {
                    goto TERMINATE;
                }
            } while (discreteStatesNeedUpdate);

            CALL(FMI3EnterContinuousTimeMode(S));

            settings->solverReset(solver, time);

            // record outputs after the event
            CALL(FMISample(S, time, result));
        }

        // record outputs
        CALL(FMISample(S, time, result));
    }

TERMINATE:
    return status;
}
