#include <stdlib.h>
#include <math.h>

#include "fmusim.h"
#include "fmusim_fmi3.h"
#include "fmusim_fmi3_me.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3ME(
    FMIInstance* S, 
    const FMIModelDescription* modelDescription, 
    const char* resourcePath,
    FMIRecorder* result,
    const FMUStaticInput * input,
    const FMISimulationSettings * settings) {

    FMIStatus status = FMIOK;

    fmi3Float64 time;

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

    size_t nSteps;
    fmi3Float64 nextRegularPoint;
    fmi3Float64 nextCommunicationPoint;
    fmi3Float64 nextInputEventTime;

    fmi3Boolean nominalsChanged = fmi3False;
    fmi3Boolean statesChanged = fmi3False;

    Solver* solver = NULL;

    CALL(FMI3InstantiateModelExchange(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False                              // loggingOn
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
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, time, fmi3True, settings->stopTime));
    CALL(FMI3ExitInitializationMode(S));

    // intial event iteration
    nominalsChanged = fmi3False;
    statesChanged = fmi3False;

    do {

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

        nominalsChanged |= nominalsOfContinuousStatesChanged;
        statesChanged |= valuesOfContinuousStatesChanged;

    } while (discreteStatesNeedUpdate);

    if (!nextEventTimeDefined) {
        nextEventTime = INFINITY;
    }

    CALL(FMI3EnterContinuousTimeMode(S));

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

        timeEvent = nextCommunicationPoint >= nextEventTime;

        if (inputEvent || timeEvent) {
            nextCommunicationPoint = min(nextInputEventTime, nextEventTime);
        }

        CALL(settings->solverStep(solver, nextCommunicationPoint, &time, &stateEvent));

        CALL(FMI3SetTime(S, time));

        CALL(FMIApplyInput(S, input, time,
            false, // discrete
            true,  // continous
            false  // after event
        ));

        if (time == nextRegularPoint) {
            nSteps++;
        }

        CALL(FMI3CompletedIntegratorStep(S, fmi3True, &stepEvent, &terminateSimulation));

        if (terminateSimulation) {
            goto TERMINATE;
        }

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

            nominalsChanged = fmi3False;
            statesChanged = fmi3False;

            do {

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

                nominalsChanged |= nominalsOfContinuousStatesChanged;
                statesChanged |= valuesOfContinuousStatesChanged;

            } while (discreteStatesNeedUpdate);

            if (!nextEventTimeDefined) {
                nextEventTime = INFINITY;
            }

            CALL(FMI3EnterContinuousTimeMode(S));

            settings->solverReset(solver, time);
        }

    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI3Terminate(S);

        if (terminateStatus != FMIFatal) {
            FMI3FreeInstance(S);
        }
    }

    if (solver) {
        settings->solverFree(solver);
    }

    return status;
}
