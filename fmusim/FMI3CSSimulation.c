#include <stdlib.h>
#include <math.h>

#include "FMIUtil.h"
#include "FMI3.h"
#include "FMI3CSSimulation.h"


#define FMI_PATH_MAX 4096

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
        // TODO: handle return code
        FMISample(instance, intermediateUpdateTime, recorder);
    }

    *earlyReturnRequested = fmi3False;
}

FMIStatus FMI3CSSimulate(const FMISimulationSettings* s) {

    FMIStatus status = FMIOK;

    FMIInstance* S = s->S;
    const bool canHandleVariableCommunicationStepSize = s->modelDescription->coSimulation->canHandleVariableCommunicationStepSize;

    fmi3Boolean inputEvent = fmi3False;
    fmi3Boolean eventEncountered = fmi3False;
    fmi3Boolean terminateSimulation = fmi3False;
    fmi3Boolean earlyReturn = fmi3False;
    fmi3Float64 lastSuccessfulTime = s->startTime;
    fmi3Float64 time = s->startTime;
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

    char resourcePath[FMI_PATH_MAX] = "";

#ifdef _WIN32
    snprintf(resourcePath, FMI_PATH_MAX, "%s\\resources\\", s->unzipdir);
#else
    snprintf(resourcePath, FMI_PATH_MAX, "%s/resources/", s->unzipdir);
#endif

    if (s->recordIntermediateValues) {
        requiredIntermediateVariables = (fmi3ValueReference*)s->recorder->valueReferences;
        nRequiredIntermediateVariables = s->recorder->nVariables;
        intermediateUpdate = recordIntermediateValues;
        S->userData = s->recorder;
    }

    CALL(FMI3InstantiateCoSimulation(S,
        s->modelDescription->instantiationToken,
        resourcePath,
        s->visible,
        s->loggingOn,
        s->eventModeUsed,
        s->earlyReturnAllowed,
        requiredIntermediateVariables,
        nRequiredIntermediateVariables,
        intermediateUpdate
    ));

    if (s->initialFMUStateFile) {
        CALL(FMIRestoreFMUStateFromFile(S, s->initialFMUStateFile));
    }

    CALL(FMIApplyStartValues(S, s));

    if (!s->initialFMUStateFile) {

        CALL(FMI3EnterInitializationMode(S, s->tolerance > 0, s->tolerance, s->startTime, s->setStopTime, s->stopTime));

        CALL(FMIApplyInput(S, s->input, s->startTime, true, true, true));

        CALL(FMI3ExitInitializationMode(S));

        if (s->eventModeUsed) {
     
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

            CALL(FMI3EnterStepMode(S));
        }
    }

    CALL(FMIRecorderUpdateSizes(s->initialRecorder));
    CALL(FMIRecorderUpdateSizes(s->recorder));

    CALL(FMISample(S, time, s->initialRecorder));
    CALL(FMISample(S, time, s->recorder));

    size_t nSteps = 0;
    bool inputApplied = false;

    for (;;) {

        if (time > s->stopTime || FMIIsClose(time, s->stopTime)) {
            break;
        }

        nextRegularPoint = s->startTime + (nSteps + 1) * s->outputInterval;

        nextCommunicationPoint = nextRegularPoint;

        nextInputEventTime = FMINextInputEvent(s->input, time);

        if (canHandleVariableCommunicationStepSize &&
            nextCommunicationPoint > nextInputEventTime &&
            !FMIIsClose(nextCommunicationPoint, nextInputEventTime)) {

            nextCommunicationPoint = nextInputEventTime;
        }

        if (nextCommunicationPoint > s->stopTime && !FMIIsClose(nextCommunicationPoint, s->stopTime)) {

            if (canHandleVariableCommunicationStepSize) {
                nextCommunicationPoint = s->stopTime;
            } else {
                break;
            }
        }

        inputEvent = FMIIsClose(nextCommunicationPoint, nextInputEventTime);

        stepSize = nextCommunicationPoint - time;

        CALL(FMIApplyInput(S, s->input, time,
            !inputApplied,     // discrete
            !inputApplied,     // continuous
            !s->eventModeUsed  // afterEvent
        ));

        CALL(FMI3DoStep(S, 
            time,                  // currentCommunicationPoint
            stepSize,              // communicationStepSize
            fmi3True,              // noSetFMUStatePriorToCurrentPoint
            &eventEncountered,     // eventHandlingNeeded
            &terminateSimulation,  // terminateSimulation
            &earlyReturn,          // earlyReturn
            &lastSuccessfulTime    // lastSuccessfulTime
        ));

        if (earlyReturn && !s->earlyReturnAllowed) {
            FMILogError("The FMU returned early from fmi3DoStep() but early return is not allowed.");
            status = FMIError;
            goto TERMINATE;
        }

        if (earlyReturn && lastSuccessfulTime < nextCommunicationPoint) {
            time = lastSuccessfulTime;
        } else { 
            time = nextCommunicationPoint;
        }

        if (FMIIsClose(time, nextRegularPoint)) {
            nSteps++;
        }

        CALL(FMISample(S, time, s->recorder));

        if (terminateSimulation) {
            goto TERMINATE;
        }

        if (s->eventModeUsed && (inputEvent || eventEncountered)) {

            CALL(FMI3EnterEventMode(S));

            if (inputEvent) {
                CALL(FMIApplyInput(S, s->input, time,
                    true,  // discrete
                    true,  // continuous
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
                    CALL(FMISample(S, time, s->recorder));
                    goto TERMINATE;
                }

            } while (discreteStatesNeedUpdate);

            CALL(FMI3EnterStepMode(S));

            CALL(FMISample(S, time, s->recorder));

            inputApplied = true;

        } else {
            inputApplied = false;
        }

        if (s->stepFinished && !s->stepFinished(s, time)) {
            break;
        }
    }

    if (s->finalFMUStateFile) {
        CALL(FMISaveFMUStateToFile(S, s->finalFMUStateFile));
    }

TERMINATE:

    if (status < FMIError) {

        const FMIStatus terminateStatus = FMI3Terminate(S);

        if (terminateStatus > status) {
            status = terminateStatus;
        }
    }

    if (status != FMIFatal && S->component) {
        FMI3FreeInstance(S);
    }

    return status;
}
