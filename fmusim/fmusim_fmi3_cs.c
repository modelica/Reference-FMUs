#include <stdlib.h>
#include <math.h>

#include "FMIUtil.h"

#include "fmusim_fmi3_cs.h"


#define CALL(f) do { status = f; if (status > FMIOK) goto TERMINATE; } while (0)


static void recordIntermediateValues(
    fmi3InstanceEnvironment instanceEnvironment,
    fmi3Float64  intermediateUpdateTime,
    fmi3Boolean  intermediateVariableSetRequested,
    fmi3Boolean  intermediateVariableGetAllowed,
    fmi3Boolean  intermediateStepFinished,
    fmi3Boolean  canReturnEarly,
    fmi3Boolean* earlyReturnRequested,
    fmi3Float64* earlyReturnTime) {

    FMIInstance* instance = (FMIInstance*)instanceEnvironment;

    FMIRecorder* recorder = (FMIRecorder*)instance->userData;

    if (intermediateVariableGetAllowed) {
        FMISample(instance, intermediateUpdateTime, recorder);
    }

    *earlyReturnRequested = fmi3False;
}

FMIStatus simulateFMI3CS(FMIInstance* S,
    const FMIModelDescription * modelDescription,
    const char* resourcePath,
    FMIRecorder* recorder,
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

    fmi3ValueReference* requiredIntermediateVariables = NULL;
    size_t nRequiredIntermediateVariables = 0;
    fmi3IntermediateUpdateCallback intermediateUpdate = NULL;

    if (settings->recordIntermediateValues) {

        nRequiredIntermediateVariables = recorder->nVariables;

        CALL(FMICalloc((void**)&requiredIntermediateVariables, nRequiredIntermediateVariables, sizeof(fmi3ValueReference)));
        
        for (size_t i = 0; i < nRequiredIntermediateVariables; i++) {
            requiredIntermediateVariables[i] = recorder->variables[i]->valueReference;
        }

        intermediateUpdate = recordIntermediateValues;

        S->userData = recorder;
    }

    CALL(FMI3InstantiateCoSimulation(S,
        modelDescription->instantiationToken,  // instantiationToken
        resourcePath,                          // resourcePath
        fmi3False,                             // visible
        fmi3False,                             // loggingOn
        settings->eventModeUsed,               // eventModeUsed
        settings->earlyReturnAllowed,          // earlyReturnAllowed
        requiredIntermediateVariables,         // requiredIntermediateVariables
        nRequiredIntermediateVariables,        // nRequiredIntermediateVariables
        intermediateUpdate                     // intermediateUpdate
    ));

    free(requiredIntermediateVariables);

    if (settings->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, settings->initialFMUStateFile));
    }

    CALL(applyStartValues(S, settings));
    CALL(FMIApplyInput(S, input, settings->startTime, true, true, false));

    if (!settings->initialFMUStateFile) {

        CALL(FMI3EnterInitializationMode(S, settings->tolerance > 0, settings->tolerance, settings->startTime, fmi3False, 0));
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
    }

    CALL(FMISample(S, time, recorder));

    size_t nSteps = 0;

    for (;;) {

        if (time >= settings->stopTime) {
            break;
        }

        nextRegularPoint = settings->startTime + (nSteps + 1) * settings->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(input, time);

        inputEvent = nextCommunicationPoint >= nextInputEventTime;

        if (inputEvent) {
            nextCommunicationPoint = nextInputEventTime;
        }

        stepSize = nextCommunicationPoint - time;

        CALL(FMIApplyInput(S, input, time,
            !settings->eventModeUsed,  // discrete
            true,                      // continuous
            !settings->eventModeUsed   // afterEvent
        ));

        CALL(FMI3DoStep(S, 
            time,                  // currentCommunicationPoint
            stepSize,              // communicationStepSize
            fmi3True,              // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,     // eventEncountered
            &terminateSimulation,  // terminateSimulation
            &earlyReturn,          // earlyReturn
            &lastSuccessfulTime    // lastSuccessfulTime
        ));

        if (earlyReturn && !settings->earlyReturnAllowed) {
            FMILogError("The FMU returned early from fmi3DoStep() but early return is not allowed.");
            status = FMIError;
            goto TERMINATE;
        }

        if (earlyReturn && lastSuccessfulTime < nextCommunicationPoint) {
            time = lastSuccessfulTime;
        } else { 
            time = nextCommunicationPoint;
        }

        if (time == nextRegularPoint) {
            nSteps++;
        }

        CALL(FMISample(S, time, recorder));

        if (terminateSimulation) {
            goto TERMINATE;
        }

        if (settings->eventModeUsed && (inputEvent || eventEncountered)) {

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
                    CALL(FMISample(S, time, recorder));
                    goto TERMINATE;
                }

            } while (discreteStatesNeedUpdate);

            if (!nextEventTimeDefined) {
                nextEventTime = INFINITY;
            }

            CALL(FMI3EnterStepMode(S));

            CALL(FMISample(S, time, recorder));
        }
    }

    if (settings->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, settings->finalFMUStateFile));
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI3Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal) {
        FMI3FreeInstance(S);
    }

    return status;
}