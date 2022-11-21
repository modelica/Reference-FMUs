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
    fmi3Float64 nextTime = 0.0;
    fmi3Float64 stepSize = 0.0;
    fmi3Float64 nextInputEventTime = INFINITY;

    CALL(FMI3InstantiateCoSimulation(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False,                             // loggingOn
        fmi3False,                             // eventModeUsed
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

    size_t nSteps = 0;

    for (;;) {

        CALL(FMISample(S, time, result));

        if (time >= settings->stopTime) {
            break;
        }

        CALL(FMIApplyInput(S, input, time, true, true, false));

        nextInputEventTime = FMINextInputEvent(input, time);

        nextTime = settings->startTime + (nSteps + 1) * settings->outputInterval;

        inputEvent = nextTime > nextInputEventTime;

        stepSize = inputEvent ? nextInputEventTime : nextTime - time;
        
        CALL(FMI3DoStep(S, 
            time, 
            stepSize,
            fmi3True, 
            &eventEncountered, 
            &terminateSimulation, 
            &earlyReturn, 
            &lastSuccessfulTime));

        if (earlyReturn && !settings->earlyReturnAllowed) {
            status = FMIError;
            goto TERMINATE;
        }

        if (terminateSimulation) {
            break;
        }

        if (earlyReturn && lastSuccessfulTime < nextTime) {
            time = lastSuccessfulTime;
        } else {
            time = nextTime;
            nSteps++;
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