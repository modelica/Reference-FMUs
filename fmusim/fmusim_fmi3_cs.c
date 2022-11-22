#include <math.h>

#include "fmusim_fmi3.h"
#include "fmusim_fmi3_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


FMIStatus simulateFMI3CS(FMIInstance* S,
    const FMIModelDescription * modelDescription,
    const char* resourcePath,
    FMIRecorder* result,
    const FMUStaticInput * input,
    const FMISimulationSettings * settings) {

    FMIStatus status = FMIOK;

    fmi3Boolean inputEvent = fmi3False;
    fmi3Boolean eventEncountered = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean earlyReturn = fmi3False;
    fmi3Float64 lastSuccessfulTime = settings->startTime;
    fmi3Float64 time = settings->startTime;
    fmi3Float64 nextCommunicationPoint = 0.0;
    fmi3Float64 nextRegularPoint = 0.0;
    fmi3Float64 stepSize = 0.0;
    fmi3Float64 nextInputEventTime = INFINITY;
    fmi3Boolean discreteStatesNeedUpdate = fmi3True;
    fmi3Boolean nominalsOfContinuousStatesChanged = fmi3False;
    fmi3Boolean valuesOfContinuousStatesChanged = fmi3False;
    fmi3Boolean nextEventTimeDefined = fmi3False;
    fmi3Float64 nextEventTime = INFINITY;

    CALL(FMI3InstantiateCoSimulation(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False,                             // loggingOn
        settings->eventModeUsed,               // eventModeUsed
        settings->earlyReturnAllowed,          // earlyReturnAllowed
        NULL,                                  // requiredIntermediateVariables
        0,                                     // nRequiredIntermediateVariables
        NULL                                   // intermediateUpdate
    ));

    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, settings->startTime, true, true, false));

    // initialize
    CALL(FMI3EnterInitializationMode(S, fmi3False, 0.0, settings->startTime, fmi3True, settings->stopTime));
    CALL(FMI3ExitInitializationMode(S));

    if (settings->eventModeUsed) {
     
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

        } while (discreteStatesNeedUpdate);

        if (!nextEventTimeDefined) {
            nextEventTime = INFINITY;
        }

        CALL(FMI3EnterStepMode(S));
    }

    size_t nSteps = 0;

    for (;;) {

        CALL(FMISample(S, time, result));

        if (time >= settings->stopTime) {
            break;
        }

        nextRegularPoint = settings->startTime + (nSteps + 1) * settings->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = nextCommunicationPoint > nextInputEventTime;

        if (inputEvent) {
            nextCommunicationPoint = nextInputEventTime;
        }

        stepSize = nextCommunicationPoint - time;

        if (settings->eventModeUsed) {
            CALL(FMIApplyInput(S, input, time, false, true, false));
        } else {
            CALL(FMIApplyInput(S, input, time, true, true, true));
        }

        CALL(FMI3DoStep(S, 
            time,                  // currentCommunicationPoint
            stepSize,              // communicationStepSize
            fmi3True,              // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,     // eventEncountered
            &terminateSimulation,  // terminate
            &earlyReturn,          // earlyReturn
            &lastSuccessfulTime    // lastSuccessfulTime
        ));

        if (earlyReturn && !settings->earlyReturnAllowed) {
            status = FMIError;
            goto TERMINATE;
        }

        if (terminateSimulation) {
            break;
        }

        if (earlyReturn && lastSuccessfulTime < nextCommunicationPoint) {
            time = lastSuccessfulTime;
        } else { 
            time = nextCommunicationPoint;
        }

        if (time == nextRegularPoint) {
            nSteps++;
        }

        if (settings->eventModeUsed && (inputEvent || eventEncountered)) {

            CALL(FMISample(S, time, result));

            CALL(FMI3EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, input, time,
                    true,  // discrete
                    true,  // continous
                    true   // after event
                ));
            }

            do {

                CALL(FMI3UpdateDiscreteStates(S,
                    &discreteStatesNeedUpdate,
                    &terminateSimulation,
                    &nominalsOfContinuousStatesChanged,
                    &valuesOfContinuousStatesChanged,
                    &nextEventTimeDefined,
                    &nextEventTime));

                if (terminateSimulation) {
                    break;
                }

            } while (discreteStatesNeedUpdate);

            if (!nextEventTimeDefined) {
                nextEventTime = INFINITY;
            }

            CALL(FMI3EnterStepMode(S));
        }
    }

TERMINATE:

    if (status != FMIFatal) {

        const FMIStatus terminateStatus = FMI3Terminate(S);

        if (terminateStatus != FMIFatal) {
            FMI3FreeInstance(S);
        }
    }

    return status;
}